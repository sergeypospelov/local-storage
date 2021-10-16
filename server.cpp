#include "kv.pb.h"
#include "protocol.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

static_assert(EAGAIN == EWOULDBLOCK);

using namespace NProtocol;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr int verbosity = 3;

void log(int level, const std::string& tag, const std::string& message)
{
    if (level <= verbosity) {
        // TODO log current time
        std::cerr << "[" << tag << "] " << message << "\n";
    }
}

void log(int level, const std::string& tag, const std::stringstream& message)
{
    log(level, tag, message.str());
}

template <class T>
void log_debug(const T& message)
{
    log(4, "D", message);
}

template <class T>
void log_info(const T& message)
{
    log(3, "I", message);
}

template <class T>
void log_error(const T& message)
{
    log(1, "E", message);
}

////////////////////////////////////////////////////////////////////////////////

constexpr int max_events = 32;

auto create_and_bind(std::string const& port)
{
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = AI_PASSIVE; /* All interfaces */

    struct addrinfo* result;
    int sockt = getaddrinfo(nullptr, port.c_str(), &hints, &result);
    if (sockt != 0) {
        log_error("getaddrinfo failed");
        return -1;
    }

    struct addrinfo* rp = nullptr;
    int socketfd = 0;
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        socketfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socketfd == -1) {
            continue;
        }

        sockt = bind(socketfd, rp->ai_addr, rp->ai_addrlen);
        if (sockt == 0) {
            break;
        }

        close(socketfd);
    }

    if (rp == nullptr) {
        log_error("bind failed");
        return -1;
    }

    freeaddrinfo(result);

    return socketfd;
}

////////////////////////////////////////////////////////////////////////////////

auto make_socket_nonblocking(int socketfd)
{
    int flags = fcntl(socketfd, F_GETFL, 0);
    if (flags == -1) {
        log_error("fcntl failed (F_GETFL)");
        return false;
    }

    flags |= O_NONBLOCK;
    int s = fcntl(socketfd, F_SETFL, flags);
    if (s == -1) {
        log_error("fcntl failed (F_SETFL)");
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

struct SocketState
{
    int fd = 0;

    Message current_message;

    std::deque<std::string> response_queue;

    uint32_t current_response_sent_count = 0;
    std::string current_response;
};

using SocketStatePtr = std::shared_ptr<SocketState>;

////////////////////////////////////////////////////////////////////////////////

SocketStatePtr accept_connection(
    int socketfd,
    struct epoll_event& event,
    int epollfd)
{
    struct sockaddr in_addr;
    socklen_t in_len = sizeof(in_addr);
    int infd = accept(socketfd, &in_addr, &in_len);
    if (infd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return nullptr;
        } else {
            log_error("accept failed");
            return nullptr;
        }
    }

    std::string hbuf(NI_MAXHOST, '\0');
    std::string sbuf(NI_MAXSERV, '\0');
    auto ret = getnameinfo(
        &in_addr, in_len,
        const_cast<char*>(hbuf.data()), hbuf.size(),
        const_cast<char*>(sbuf.data()), sbuf.size(),
        NI_NUMERICHOST | NI_NUMERICSERV);

    if (ret == 0) {
        std::stringstream log_message;
        log_message << "accepted connection on fd " << infd
            << "(host=" << hbuf << ", port=" << sbuf << ")";
        log_info(log_message);
    }

    if (!make_socket_nonblocking(infd)) {
        log_error("make_socket_nonblocking failed");
        return nullptr;
    }

    event.data.fd = infd;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, infd, &event) == -1) {
        log_error("epoll_ctl failed");
        return nullptr;
    }

    auto state = std::make_shared<SocketState>();
    state->fd = infd;
    return state;
}

////////////////////////////////////////////////////////////////////////////////

// request -> response func
using Handler =
    std::function<std::string(char request_type, const std::string& request)>;

////////////////////////////////////////////////////////////////////////////////

bool process_requests(SocketState& state, const Handler& handler)
{
    bool success = true;

    char buf[512];
    while (true) {
        auto len = std::min(sizeof(buf), state.current_message.to_read());
        auto count = recv(state.fd, buf, len, 0);

        if (count == -1) {
            if (errno != EAGAIN) {
                // TODO proper logging
                perror("send failed");

                success = false;
            }

            break;
        } else if (count == 0) {
            break;
        }

        state.current_message.on_data(buf, count);

        if (count < len) {
            break;
        }

        if (state.current_message.is_complete()) {
            auto response = handler(
                state.current_message.request_type,
                state.current_message.buffer);

            state.response_queue.push_back(std::move(response));

            state.current_message.reset();
        }
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////

bool process_responses(SocketState& state)
{
    bool success = true;

    while (true) {
        auto& buffer = state.current_response;
        auto& offset = state.current_response_sent_count;
        auto len = buffer.size() - offset;

        const auto count = send(
            state.fd,
            buffer.data() + offset,
            len,
            MSG_NOSIGNAL);

        if (count == -1) {
            if (errno != EAGAIN) {
                // TODO proper logging
                perror("send failed");

                success = false;
            }

            break;
        } else if (count == 0) {
            break;
        }

        if (count == len) {
            if (state.response_queue.empty()) {
                break;
            }

            buffer = state.response_queue.front();
            offset = 0;
            state.response_queue.pop_front();
        }
    }

    return success;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char** argv)
{
    if (argc < 2) {
        return 1;
    }

    /*
     * socket creation and epoll boilerplate
     * TODO extract into struct Bootstrap
     */

    auto socketfd = ::create_and_bind(argv[1]);
    if (socketfd == -1) {
        return 1;
    }

    if (!::make_socket_nonblocking(socketfd)) {
        return 1;
    }

    if (listen(socketfd, SOMAXCONN) == -1) {
        log_error("listen failed");
        return 1;
    }

    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        log_error("epoll_create1 failed");
        return 1;
    }

    struct epoll_event event;
    event.data.fd = socketfd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event) == -1) {
        log_error("epoll_ctl failed");
        return 1;
    }

    /*
     * handler function
     */

    auto handle_get = [&] (const std::string& request) {
        NProto::TGetRequest get_request;
        if (!get_request.ParseFromArray(request.data(), request.size())) {
            // TODO proper handling

            abort();
        }

        // TODO proper handling
        std::stringstream log_message;
        log_message << "get_request: " << get_request.DebugString();
        log_info(log_message);

        NProto::TGetResponse get_response;
        get_response.set_request_id(get_request.request_id());
        get_response.set_offset(111);

        std::stringstream response;
        get_response.SerializeToOstream(&response);

        return response.str();
    };

    auto handle_put = [&] (const std::string& request) {
        NProto::TPutRequest put_request;
        if (!put_request.ParseFromArray(request.data(), request.size())) {
            // TODO proper handling

            abort();
        }

        // TODO proper handling
        std::stringstream log_message;
        log_message << "put_request: " << put_request.DebugString();
        log_info(log_message);

        NProto::TPutResponse put_response;
        put_response.set_request_id(put_request.request_id());

        std::stringstream response;
        put_response.SerializeToOstream(&response);

        return response.str();
    };

    Handler handler = [&] (char request_type, const std::string& request) {
        switch (request_type) {
            case PUT_REQUEST: return handle_put(request);
            case GET_REQUEST: return handle_get(request);
        }

        // TODO proper handling

        abort();
        return std::string();
    };

    /*
     * rpc state and event loop
     * TODO extract into struct Rpc
     */

    std::array<struct epoll_event, ::max_events> events;
    std::unordered_map<int, SocketStatePtr> states;

    auto finalize = [&] (int fd) {
        std::stringstream log_message;
        log_message << "close " << fd;
        log_info(log_message);

        close(fd);
        states.erase(fd);
    };

    while (true) {
        const auto n = epoll_wait(epollfd, events.data(), ::max_events, -1);

        {
            std::stringstream log_message;
            log_message << "got " << n << " events";
            log_info(log_message);
        }

        for (int i = 0; i < n; ++i) {
            const auto fd = events[i].data.fd;

            if (events[i].events & EPOLLERR
                    || events[i].events & EPOLLHUP
                    || !(events[i].events & (EPOLLIN | EPOLLOUT)))
            {
                std::stringstream log_message;
                log_message << "epoll event error on fd " << fd;
                log_error(log_message);

                finalize(fd);

                continue;
            }

            if (socketfd == fd) {
                while (true) {
                    auto state = ::accept_connection(socketfd, event, epollfd);
                    if (!state) {
                        break;
                    }

                    states[state->fd] = state;
                }

                continue;
            }

            if (events[i].events & EPOLLIN) {
                auto state = states.at(fd);
                if (!process_requests(*state, handler)) {
                    finalize(fd);
                }
            }

            if (events[i].events & EPOLLOUT) {
                auto state = states.at(fd);
                if (!process_responses(*state)) {
                    finalize(fd);
                }
            }
        }
    }

    log_info("exiting");

    close(socketfd);

    return 0;
}
