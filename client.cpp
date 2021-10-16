#include "kv.pb.h"

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

// TODO use protocol.h, currently this code is not compatible with server code

static_assert(EAGAIN == EWOULDBLOCK);

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr int max_events = 32;
constexpr int timeout = 1000;
constexpr int buf_size = 1024;

////////////////////////////////////////////////////////////////////////////////

bool send_request(int fd, const std::string& data)
{
    auto count = send(fd, data.c_str(), data.size(), 0);
    if (count == -1) {
        if (errno == EAGAIN) {
            return false;
        }
    } else if (count == 0) {
        std::cerr << "[I] close " << fd << "\n";
        close(fd);
        return false;
    }

    return true;
}

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
        std::cerr << "[E] epoll_create1 failed\n";
        return 1;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    event.data.fd = socketfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event) == -1) {
        std::cerr << "[E] epoll_ctl failed\n";
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

    std::array<struct epoll_event, ::max_events> events;

    int num_ready = epoll_wait(epollfd, events.data(), max_events, timeout);
    for (int i = 0; i < num_ready; i++) {
        if (events[i].events & EPOLLIN) {
            std::cerr << "[I] socket " << events[i].data.fd
                << " connected\n";

            // TODO check that fd == socketfd
        }
    }

    char buffer[buf_size];

    uint64_t request_id = 0;
    uint64_t offset = 0;
    int inflight = 0;

    while (request_id < max_requests || inflight) {
        num_ready = epoll_wait(epollfd, events.data(), max_events, timeout);
        for (int i = 0; i < num_ready; i++) {
            if (events[i].events & EPOLLIN) {
                std::cerr << "Socket " << events[i].data.fd
                    << " got some data\n";

                // TODO check that fd == socketfd

                bzero(buffer, buf_size);
                recv(socketfd, buffer, buf_size, 0);
                // TODO check recv return value

                std::cerr << "[I] received: " << buffer << "\n";

                --inflight;
            }

            if ((events[i].events & EPOLLOUT) && request_id < max_requests) {
                std::stringstream key;
                key << "key" << request_id;

                NProto::TPutRequest put_request;
                put_request.set_request_id(request_id);
                put_request.set_key(key.str());
                put_request.set_offset(offset);

                // TODO send message type

                std::stringstream message;
                put_request.SerializeToOstream(&message);

                auto count = send(
                    socketfd,
                    message.str().c_str(),
                    message.str().size(),
                    0);

                if (count == -1) {
                    perror("[E] send failed");
                    close(socketfd);
                    return 1;
                } else if (count == 0) {
                    std::cerr << "[I] close " << socketfd << "\n";
                    close(socketfd);
                    return 1;
                } else if (count < message.str().size()) {
                    // TODO proper handling

                    abort();
                }

                ++request_id;
                offset += 4;    // +4 doesn't actually matter

                ++inflight;
            }
        }
    }

    close(socketfd);

    return 0;
}
