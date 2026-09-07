#include "httprequest.h"
#include "socket.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <stdexcept>
#include <type_traits>
#include <utility>
SSL_CTX *Socket::ssl_ctx = nullptr;
void Check(bool ok) {
  if (!ok)
    throw std::runtime_error("check failed");
}
int Fds() {
  auto d = opendir("/proc/self/fd");
  if (!d)
    throw std::runtime_error("fd inspection failed");
  int n = 0;
  while (readdir(d))
    ++n;
  closedir(d);
  return n;
}
struct Fd {
  int value;
  ~Fd() {
    if (value >= 0)
      close(value);
  }
};
int main(int argc, char **argv) {
  int result = 0;
  try {
    static_assert(!std::is_copy_constructible_v<Socket>);
    static_assert(std::is_nothrow_move_constructible_v<Socket>);
    std::string group = argc > 1 ? argv[1] : "";
    if (group == "request") {
      httprequest r("text/plain", std::string("a\0b", 3));
      auto wire = r.ToString("localhost", "/test");
      Check(
          wire ==
          std::string(
              "POST /test HTTP/1.1\r\nHost: localhost\r\nContent-Type: "
              "text/plain\r\nContent-Length: 3\r\nConnection: close\r\n\r\n") +
              std::string("a\0b", 3));
    } else if (group == "lifecycle") {
      int count = Fds();
      {
        Socket a;
        Check(a.isValid());
        Socket b(std::move(a));
        Check(!a.isValid() && b.isValid());
        Socket c;
        c = std::move(b);
        Check(!b.isValid() && c.isValid());
        Socket &same = c;
        c = std::move(same);
        Check(c.isValid());
      }
      Check(Fds() == count);
    } else if (group == "loopback") {
      int count = Fds();
      {
        Fd listener{socket(AF_INET, SOCK_STREAM, 0)};
        Check(listener.value >= 0);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Check(bind(listener.value, reinterpret_cast<sockaddr *>(&address),
                   sizeof(address)) == 0);
        Check(listen(listener.value, 2) == 0);
        socklen_t length = sizeof(address);
        Check(getsockname(listener.value,
                          reinterpret_cast<sockaddr *>(&address),
                          &length) == 0);
        Socket client;
        Check(client.Connect("127.0.0.1",
                             std::to_string(ntohs(address.sin_port))) == 0);
        Fd peer{accept(listener.value, nullptr, nullptr)};
        Check(peer.value >= 0);
        Check(client.Send("ping") == 4);
        char buffer[4];
        Check(recv(peer.value, buffer, 4, MSG_WAITALL) == 4);
        Check(std::string(buffer, 4) == "ping");
        Check(send(peer.value, "pong", 4, 0) == 4);
        Check(shutdown(peer.value, SHUT_WR) == 0);
        Check(client.Receive(MSG_WAITALL) == "pong");
      }
      Check(Fds() == count);
    } else
      throw std::runtime_error("unknown group");
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    result = 1;
  }
  SSL_CTX_free(Socket::ssl_ctx);
  Socket::ssl_ctx = nullptr;
  return result;
}
