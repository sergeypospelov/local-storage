#include <array>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include <errno.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <resolv.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define MAXBUF 1024
#define MAX_EPOLL_EVENTS 64

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr int max_events = 32;
constexpr int timeout = 1000;
constexpr int buf_size = 1024;

////////////////////////////////////////////////////////////////////////////////

auto on_request(int fd)
{
    char buf[512];
    auto count = read(fd, buf, 512);
    if (count == -1) {
        if (errno == EAGAIN) {
            return false;
        }
    } else if (count == 0) {
        std::cerr << "[I] Close " << fd << "\n";
        close(fd);
        return false;
    }

    std::cerr << fd << " says: " <<  buf;

    auto response_data = "ok\n";
    count = write(fd, response_data, 3);
    std::cerr << "written: " << count << std::endl;
    // TODO check count

    return true;
}

}   // namespace

int main(int argc, const char** argv) {
    if (argc < 2) {
        return 1;
    }

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
    dest.sin_port = htons(atoi(argv[1]));

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
                << " connected" << std::endl;

            // TODO check that fd == socketfd
        }
    }

    char buffer[buf_size];

    int received_count = 0;

    while (received_count < 100) {
        num_ready = epoll_wait(epollfd, events.data(), max_events, timeout);
        for (int i = 0; i < num_ready; i++) {
            if (events[i].events & EPOLLIN) {
                std::cerr << "Socket " << events[i].data.fd
                    << " got some data" << std::endl;

                // TODO check that fd == socketfd

                bzero(buffer, buf_size);
                recv(socketfd, buffer, buf_size, 0);
                // TODO check recv return value

                std::cerr << "[I] received: " << buffer << std::endl;

                ++received_count;
            } else if (events[i].events & EPOLLOUT) {
                std::string message = "request";
                send(socketfd, message.c_str(), message.size(), 0);
                // TODO check send return value
            }
        }
    }

    close(socketfd);

    return 0;
}
