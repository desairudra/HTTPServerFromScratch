#include "HttpResponse.h"
#include "ThreadPool.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <netdb.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#pragma once
constexpr int MAX_EVENTS = 1000;
constexpr int MAX_CLIENTS = 1000;
constexpr int PORT = 8080;
using namespace std;
class Server {
public:
  Server(int port) { this->port = port; }
  void checkTimeouts() {
    time_t now = time(nullptr);

    for (auto it = lastActivity.begin(); it != lastActivity.end();) {
      if (now - it->second > 10) {
        close(it->first);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, it->first, nullptr);
        fd_ip.erase(it->first);
        it = lastActivity.erase(it);
      } else {
        ++it;
      }
    }
  }
  void handleclient(ClientInfo clientsocket, thread::id id) {

    char buffer[4096];
    int bytesread;

    HttpRequest request;
    while (true) {
      bytesread = recv(clientsocket.socket, buffer, sizeof(buffer) - 1, 0);

      if (bytesread == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          return;
        }

        perror("recv");
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientsocket.socket, nullptr);

        close(clientsocket.socket);
        fd_ip.erase(clientsocket.socket);
        lastActivity.erase(clientsocket.socket);
        return;

      } else if (bytesread == 0) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientsocket.socket, nullptr);
        close(clientsocket.socket);
        fd_ip.erase(clientsocket.socket);
        lastActivity.erase(clientsocket.socket);
        return;
      }
      clientsocket.recvBuffer += string(buffer, bytesread);

      if (clientsocket.recvBuffer.find("\r\n\r\n") == string::npos) {
        continue;
      }
      break;
    }
    lastActivity[clientsocket.socket] = time(nullptr);

    cout << "HTTP request received:" << endl;

    request.parse(clientsocket.recvBuffer);
    HttpResponse response(request.get_path());
    string responsestring = response.genrate_response(
        clientsocket, request.get_connection_status(), id, clientsocket.socket);
    int send_bytes = 0;
    clientsocket.sendBuffer = responsestring;
    if (request.get_connection_status() == "close") {

      int bytessend = send(clientsocket.socket, responsestring.c_str(),
                           responsestring.size(), 0);
      if (bytessend == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {

          return;
        }

        perror("send");

        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientsocket.socket, nullptr);

        close(clientsocket.socket);

        fd_ip.erase(clientsocket.socket);
        lastActivity.erase(clientsocket.socket);

        return;
      }

      if (epoll_ctl(epollFd, EPOLL_CTL_DEL, clientsocket.socket, nullptr) ==
          -1) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientsocket.socket, nullptr);
        close(clientsocket.socket);
        fd_ip.erase(clientsocket.socket);
        lastActivity.erase(clientsocket.socket);
      }
      close(clientsocket.socket);
      fd_ip.erase(clientsocket.socket);
      lastActivity.erase(clientsocket.socket);
      return;
    }
    int bytessend = send(clientsocket.socket, responsestring.c_str(),
                         responsestring.size(), 0);

    if (bytessend == -1) {
      perror("send");

      epoll_ctl(epollFd, EPOLL_CTL_DEL, clientsocket.socket, nullptr);

      close(clientsocket.socket);

      fd_ip.erase(clientsocket.socket);
      lastActivity.erase(clientsocket.socket);
      return;
    }

    epoll_event ev;
    ev.events = EPOLLIN | EPOLLONESHOT;
    ev.data.fd = clientsocket.socket;

    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientsocket.socket, &ev) == -1) {
      epoll_ctl(epollFd, EPOLL_CTL_DEL, clientsocket.socket, nullptr);
      close(clientsocket.socket);
      fd_ip.erase(clientsocket.socket);
      lastActivity.erase(clientsocket.socket);
    }
    lastActivity[clientsocket.socket] = time(nullptr);
  }

  void start() {
    int status, clientsocket, yes = 1, bytesread;
    struct addrinfo hints;
    struct addrinfo *serverinfo;
    struct sockaddr_storage their_addr;
    struct epoll_event event, events[MAX_EVENTS];
    socklen_t addr_size;
    HttpRequest request;
    ThreadPool pool(8, this);

    char buffer[4096];

    string responsestring;
    string portStr = to_string(port);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((status = getaddrinfo(NULL, portStr.c_str(), &hints, &serverinfo)) !=
        0) {
      fprintf(stderr, "gai error: %s\n", gai_strerror(status));
      exit(1);
    }
    sockfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, 0);
    if (sockfd == -1) {
      perror("server: socket");
      exit(1);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      exit(1);
    }
    freeaddrinfo(serverinfo);
    if (listen(sockfd, 20) == -1) {
      close(sockfd);
      perror("listen");
      exit(1);
    }
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    epollFd = epoll_create1(0);

    if (epollFd == -1) {
      std::cerr << "Failed to create epoll instance." << std::endl;
      close(sockfd);
      exit(1);
    }
    event.events = EPOLLIN;
    event.data.fd = sockfd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, sockfd, &event) == -1) {
      std::cerr << "Failed to add server socket to epoll instance."
                << std::endl;
      close(sockfd);
      close(epollFd);
      exit(1);
    }
    cout << "Server listening on port " << this->port << endl;

    while (true) {

      int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, 1000);
      if (numEvents == -1) {
        std::cerr << "Failed to wait for events." << std::endl;
        break;
      }
      for (int i = 0; i < numEvents; i++) {
        if (events[i].data.fd == sockfd) {
          while (true) {
            addr_size = sizeof(their_addr);
            clientsocket =
                accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
            if (clientsocket == -1) {
              if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
              }

              perror("accept");
              break;
            }
            int clientflags = fcntl(clientsocket, F_GETFL, 0);
            if (clientflags == -1) {
              perror("fcntl(F_GETFL)");
              close(clientsocket);
              fd_ip.erase(clientsocket);
              continue;
            }
            if (fcntl(clientsocket, F_SETFL, clientflags | O_NONBLOCK) == -1) {
              perror("fcntl(F_GETFL)");
              close(clientsocket);
              fd_ip.erase(clientsocket);
              continue;
            }
            event.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP;
            event.data.fd = clientsocket;
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientsocket, &event) == -1) {
              std::cerr << "Failed to add client socket to epoll instance."
                        << std::endl;
              close(clientsocket);
              continue;
            }

            cout << "Client Connected" << endl;
            if (their_addr.ss_family == AF_INET) {
              sockaddr_in *ipv4 = (sockaddr_in *)&their_addr;
              inet_ntop(AF_INET, &ipv4->sin_addr, buffer, sizeof(buffer));
              Logger::info("client connected", string(buffer));

            } else {
              sockaddr_in6 *ipv6 = (sockaddr_in6 *)&their_addr;
              inet_ntop(AF_INET6, &ipv6->sin6_addr, buffer, sizeof(buffer));
              Logger::info("client connected", string(buffer));
            }
            lastActivity[clientsocket] = time(nullptr);
            fd_ip[clientsocket] = string(buffer);
          }

        } else {
          ClientInfo d;
          d.ip = fd_ip[events[i].data.fd];
          d.socket = events[i].data.fd;

          if (events[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
            epoll_ctl(epollFd, EPOLL_CTL_DEL, d.socket, nullptr);

            close(d.socket);
            fd_ip.erase(d.socket);
            continue;
          } else if (events[i].events & EPOLLIN) {
            pool.enqueue(d);
          }
        }
      }
      checkTimeouts();
    }
  }

private:
  int port;
  int sockfd;
  int epollFd;
  map<long long, time_t> lastActivity;
  map<long long, string> fd_ip;
};