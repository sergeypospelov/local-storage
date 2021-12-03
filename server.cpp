#include "kv.pb.h"
#include "log.h"
#include "protocol.h"
#include "rpc.h"

#include <array>
#include <cstdio>
#include <cstring>
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

using namespace NLogging;
using namespace NProtocol;
using namespace NRpc;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr int max_events = 32;

////////////////////////////////////////////////////////////////////////////////

auto create_and_bind(std::string const &port) {
  struct addrinfo hints;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* TCP */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */

  struct addrinfo *result;
  int sockt = getaddrinfo(nullptr, port.c_str(), &hints, &result);
  if (sockt != 0) {
    LOG_ERROR("getaddrinfo failed");
    return -1;
  }

  struct addrinfo *rp = nullptr;
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
    LOG_ERROR("bind failed");
    return -1;
  }

  freeaddrinfo(result);

  return socketfd;
}

////////////////////////////////////////////////////////////////////////////////

auto make_socket_nonblocking(int socketfd) {
  int flags = fcntl(socketfd, F_GETFL, 0);
  if (flags == -1) {
    LOG_ERROR("fcntl failed (F_GETFL)");
    return false;
  }

  flags |= O_NONBLOCK;
  int s = fcntl(socketfd, F_SETFL, flags);
  if (s == -1) {
    LOG_ERROR("fcntl failed (F_SETFL)");
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

SocketStatePtr accept_connection(int socketfd, struct epoll_event &event,
                                 int epollfd) {
  struct sockaddr in_addr;
  socklen_t in_len = sizeof(in_addr);
  int infd = accept(socketfd, &in_addr, &in_len);
  if (infd == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return nullptr;
    } else {
      LOG_ERROR("accept failed");
      return nullptr;
    }
  }

  std::string hbuf(NI_MAXHOST, '\0');
  std::string sbuf(NI_MAXSERV, '\0');
  auto ret = getnameinfo(&in_addr, in_len, const_cast<char *>(hbuf.data()),
                         hbuf.size(), const_cast<char *>(sbuf.data()),
                         sbuf.size(), NI_NUMERICHOST | NI_NUMERICSERV);

  if (ret == 0) {
    LOG_INFO_S("accepted connection on fd " << infd << "(host=" << hbuf
                                            << ", port=" << sbuf << ")");
  }

  if (!make_socket_nonblocking(infd)) {
    LOG_ERROR("make_socket_nonblocking failed");
    return nullptr;
  }

  event.data.fd = infd;
  event.events = EPOLLIN | EPOLLOUT | EPOLLET;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, infd, &event) == -1) {
    LOG_ERROR("epoll_ctl failed");
    return nullptr;
  }

  auto state = std::make_shared<SocketState>();
  state->fd = infd;
  return state;
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char **argv) {
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
    LOG_ERROR("listen failed");
    return 1;
  }

  int epollfd = epoll_create1(0);
  if (epollfd == -1) {
    LOG_ERROR("epoll_create1 failed");
    return 1;
  }

  struct epoll_event event;
  event.data.fd = socketfd;
  event.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event) == -1) {
    LOG_ERROR("epoll_ctl failed");
    return 1;
  }

  /*
   * handler function
   */

  // TODO on-disk storage
  std::unordered_map<std::string, uint64_t> storage;

  auto handle_get = [&](const std::string &request) {
    NProto::TGetRequest get_request;
    if (!get_request.ParseFromArray(request.data(), request.size())) {
      // TODO proper handling

      abort();
    }

    LOG_DEBUG_S("get_request: " << get_request.ShortDebugString());

    NProto::TGetResponse get_response;
    get_response.set_request_id(get_request.request_id());
    auto it = storage.find(get_request.key());
    if (it != storage.end()) {
      get_response.set_offset(it->second);
    }

    std::stringstream response;
    serialize_header(GET_RESPONSE, get_response.ByteSizeLong(), response);
    get_response.SerializeToOstream(&response);

    return response.str();
  };

  auto handle_put = [&](const std::string &request) {
    NProto::TPutRequest put_request;
    if (!put_request.ParseFromArray(request.data(), request.size())) {
      // TODO proper handling

      abort();
    }

    LOG_DEBUG_S("put_request: " << put_request.ShortDebugString());

    storage[put_request.key()] = put_request.offset();

    NProto::TPutResponse put_response;
    put_response.set_request_id(put_request.request_id());

    std::stringstream response;
    serialize_header(PUT_RESPONSE, put_response.ByteSizeLong(), response);
    put_response.SerializeToOstream(&response);

    return response.str();
  };

  Handler handler = [&](char request_type, const std::string &request) {
    switch (request_type) {
    case PUT_REQUEST:
      return handle_put(request);
    case GET_REQUEST:
      return handle_get(request);
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

  auto finalize = [&](int fd) {
    LOG_INFO_S("close " << fd);

    close(fd);
    states.erase(fd);
  };

  while (true) {
    const auto n = epoll_wait(epollfd, events.data(), ::max_events, -1);

    { LOG_INFO_S("got " << n << " events"); }

    for (int i = 0; i < n; ++i) {
      const auto fd = events[i].data.fd;

      if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
          !(events[i].events & (EPOLLIN | EPOLLOUT))) {
        LOG_ERROR_S("epoll event error on fd " << fd);

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
        if (!process_input(*state, handler)) {
          finalize(fd);
        }
      }

      if (events[i].events & EPOLLOUT) {
        auto state = states.at(fd);
        if (!process_output(*state)) {
          finalize(fd);
        }
      }
    }
  }

  LOG_INFO("exiting");

  close(socketfd);

  return 0;
}
