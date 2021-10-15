#include "kv.pb.h"

#include <array>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace {

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
        std::cerr << "[E] getaddrinfo failed\n";
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
        std::cerr << "[E] bind failed\n";
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
        std::cerr << "[E] fcntl failed (F_GETFL)\n";
        return false;
    }

    flags |= O_NONBLOCK;
    int s = fcntl(socketfd, F_SETFL, flags);
    if (s == -1) {
        std::cerr << "[E] fcntl failed (F_SETFL)\n";
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

auto accept_connection(int socketfd, struct epoll_event& event, int epollfd)
{
    struct sockaddr in_addr;
    socklen_t in_len = sizeof(in_addr);
    int infd = accept(socketfd, &in_addr, &in_len);
    if (infd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return false;
        } else {
            std::cerr << "[E] accept failed\n";
            return false;
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
        std::cerr << "[I] Accepted connection on descriptor "
            << infd << "(host=" << hbuf << ", port=" << sbuf << ")" << "\n";
    }

    if (!make_socket_nonblocking(infd)) {
        std::cerr << "[E] make_socket_nonblocking failed\n";
        return false;
    }

    event.data.fd = infd;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, infd, &event) == -1) {
        std::cerr << "[E] epoll_ctl failed\n";
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool on_request(int fd)
{
    char buf[512];
    auto count = recv(fd, buf, 512, 0);
    if (count == -1) {
        if (errno == EAGAIN) {
            return false;
        }
    } else if (count == 0) {
        std::cerr << "[I] close " << fd << "\n";
        close(fd);
        return false;
    }

    NProto::TPutRequest put_request;
    if (!put_request.ParseFromArray(buf, count)) {
        // TODO proper handling

        abort();
    }

    std::cerr << "[I] " << fd << " put_request: " << put_request.DebugString() << "\n";

    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool send_response(int fd)
{
    static int counter = 0;

    std::stringstream message;
    message << "ok " << counter << "\n";
    auto count = send(
        fd,
        message.str().c_str(),
        message.str().size(),
        MSG_NOSIGNAL);

    if (count == -1) {
        if (errno == EAGAIN) {
            return false;
        } else {
            perror("[E] send failed");
            close(fd);
            return false;
        }
    } else if (count == 0) {
        std::cerr << "[I] close " << fd << "\n";
        close(fd);
        return false;
    } else if (count < message.str().size()) {
        // TODO proper handling

        abort();
    }

    ++counter;

    return false;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char** argv)
{
    if (argc < 2) {
        return 1;
    }

    auto socketfd = ::create_and_bind(argv[1]);
    if (socketfd == -1) {
        return 1;
    }

    if (!::make_socket_nonblocking(socketfd)) {
        return 1;
    }

    if (listen(socketfd, SOMAXCONN) == -1) {
        std::cerr << "[E] listen failed\n";
        return 1;
    }

    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        std::cerr << "[E] epoll_create1 failed\n";
        return 1;
    }

    struct epoll_event event;
    event.data.fd = socketfd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event) == -1) {
        std::cerr << "[E] epoll_ctl failed\n";
        return 1;
    }

    std::array<struct epoll_event, ::max_events> events;

    while (true) {
        auto n = epoll_wait(epollfd, events.data(), ::max_events, -1);

        std::cerr << "[I] got " << n << " events\n";

        for (int i = 0; i < n; ++i) {
            if (events[i].events & EPOLLERR
                    || events[i].events & EPOLLHUP
                    || !(events[i].events & (EPOLLIN | EPOLLOUT)))
            {
                std::cerr << "[E] epoll event error\n";
                close(events[i].data.fd);

                continue;
            }

            if (socketfd == events[i].data.fd) {
                while (::accept_connection(socketfd, event, epollfd)) {
                }

                continue;
            }

            if (events[i].events & EPOLLIN) {
                auto fd = events[i].data.fd;
                while (::on_request(fd)) {
                }

                // TODO register request in on_request
                // requests should be identified by a (fd, request_id) pair
            }

            if (events[i].events & EPOLLOUT) {
                auto fd = events[i].data.fd;
                while (::send_response(fd)) {
                }

                // TODO respond to the registered requests and clear those
                // requests
            }
        }
    }

    std::cerr << "[I] exiting\n";

    close(socketfd);

    return 0;
}
