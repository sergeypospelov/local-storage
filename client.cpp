#include "kv.pb.h"
#include "log.h"
#include "protocol.h"
#include "rpc.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include <errno.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <resolv.h>
#include <sys/epoll.h>
#include <sys/socket.h>

static_assert(EAGAIN == EWOULDBLOCK);

using namespace NLogging;
using namespace NProtocol;
using namespace NRpc;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr int max_events = 32;
constexpr int timeout = 1000;

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char** argv) {
    if (argc < 3) {
        return 1;
    }

    const auto port = atoi(argv[1]);
    const auto max_requests = atoi(argv[2]);

    int socketfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (socketfd == -1) {
        return 1;
    }

    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        log_error("epoll_create1 failed");
        return 1;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    event.data.fd = socketfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event) == -1) {
        log_error("epoll_ctl failed");
        return 1;
    }

    struct sockaddr_in dest;
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr.s_addr) == 0) {
        perror("failed to convert address");
        return errno;
    }

    if (connect(socketfd, (struct sockaddr*)&dest, sizeof(dest)) != 0) {
        if (errno != EINPROGRESS) {
            perror("failed to connect");
            return errno;
        }
    }

    SocketState state;
    state.fd = socketfd;

    /*
     * generating requests
     */

    for (int i = 0; i < max_requests; ++i) {
        std::stringstream key;
        key << "key" << i;

        NProto::TPutRequest put_request;
        put_request.set_request_id(i);
        put_request.set_key(key.str());
        put_request.set_offset(i * 4);

        std::stringstream message;
        serialize_header(PUT_REQUEST, put_request.ByteSizeLong(), message);
        put_request.SerializeToOstream(&message);

        state.output_queue.push_back(message.str());

        // std::cerr << "generated put request " << put_request.DebugString() << std::endl;
    }

    for (int i = 0; i < max_requests; ++i) {
        std::stringstream key;
        key << "key" << i;

        NProto::TGetRequest get_request;
        get_request.set_request_id(max_requests + i);
        get_request.set_key(key.str());

        std::stringstream message;
        serialize_header(GET_REQUEST, get_request.ByteSizeLong(), message);
        get_request.SerializeToOstream(&message);

        state.output_queue.push_back(message.str());

        // std::cerr << "generated get request " << get_request.DebugString() << std::endl;
    }

    /*
     * handler function
     */

    int response_count = 0;

    auto handle_get = [&] (const std::string& response) {
        NProto::TGetResponse get_response;
        if (!get_response.ParseFromArray(response.data(), response.size())) {
            // TODO proper handling

            abort();
        }

        std::stringstream log_message;
        log_message << "get_response: " << get_response.DebugString();
        log_info(log_message);

        ++response_count;

        return std::string();
    };

    auto handle_put = [&] (const std::string& response) {
        NProto::TPutResponse put_response;
        if (!put_response.ParseFromArray(response.data(), response.size())) {
            // TODO proper handling

            abort();
        }

        // TODO proper handling
        std::stringstream log_message;
        log_message << "put_response: " << put_response.DebugString();
        log_info(log_message);

        ++response_count;

        return std::string();
    };

    Handler handler = [&] (char message_type, const std::string& response) {
        switch (message_type) {
            case PUT_RESPONSE: return handle_put(response);
            case GET_RESPONSE: return handle_get(response);
        }

        // TODO proper handling

        abort();
        return std::string();
    };

    /*
     * rpc state and event loop
     */

    std::array<struct epoll_event, ::max_events> events;

    int num_ready = epoll_wait(epollfd, events.data(), max_events, timeout);
    for (int i = 0; i < num_ready; i++) {
        if (events[i].events & EPOLLIN) {
            verify(events[i].data.fd == socketfd, "fd mismatch");

            std::stringstream log_message;
            log_message << "socket " << socketfd << " connected";
            log_info(log_message);
        }
    }

    if (!process_output(state)) {
        log_error("failed to send request");
        return 3;
    }

    while (response_count < 2 * max_requests) {
        num_ready = epoll_wait(epollfd, events.data(), max_events, timeout);
        for (int i = 0; i < num_ready; i++) {
            verify(events[i].data.fd == socketfd, "fd mismatch");

            if (events[i].events & EPOLLIN) {
                if (!process_input(state, handler)) {
                    log_error("failed to read response");
                    return 2;
                }
            }

            if ((events[i].events & EPOLLOUT)) {
                if (!process_output(state)) {
                    log_error("failed to send request");
                    return 3;
                }
            }
        }
    }

    close(socketfd);

    return 0;
}
