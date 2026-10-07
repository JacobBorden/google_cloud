#include "httprequest.h"
#include "http_response.h"
#include "socket.h"
#include <arpa/inet.h>
#include <chrono>
#include <cerrno>
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
      httprequest custom;
      custom.method = "PUT";
      custom.body = "payload";
      custom.AddHeader("cOnNeCtIoN", "keep-alive");
      custom.AddHeader("X-Trace", "test");
      Check(custom.ToString("localhost", "/resource") ==
            "PUT /resource HTTP/1.1\r\nHost: localhost\r\n"
            "cOnNeCtIoN: keep-alive\r\nX-Trace: test\r\n"
            "Content-Length: 7\r\n\r\npayload");
      custom.AddHeader("content-length", "7");
      Check(custom.ToString("localhost", "/resource").find(
                "Content-Length: 7\r\n") == std::string::npos);
      custom.headers.pop_back();
      custom.AddHeader("transfer-encoding", "chunked");
      custom.body = "7\r\npayload\r\n0\r\n\r\n";
      Check(custom.ToString("localhost", "/resource").find(
                "Content-Length:") == std::string::npos);
      auto rejects_request = [](auto action) {
        try {
          action();
        } catch (const std::invalid_argument &) {
          return true;
        }
        return false;
      };
      Check(rejects_request([&] { custom.AddHeader("X-Bad\r\nInjected", "x"); }));
      Check(rejects_request([&] { custom.AddHeader("X-Bad", "x\r\nInjected: y"); }));
      Check(rejects_request([&] { custom.AddHeader("hOsT", "other.example"); }));
      custom.headers.push_back({"HOST", "other.example"});
      Check(rejects_request([&] { custom.ToString("localhost", "/resource"); }));
      custom.headers.pop_back();
      custom.headers.push_back({"X-Direct", "x\nInjected: y"});
      Check(rejects_request([&] { custom.ToString("localhost", "/resource"); }));
      custom.headers.pop_back();
      Check(rejects_request([&] { custom.ToString("host\r\nInjected: y", "/resource"); }));
      Check(rejects_request([&] { custom.ToString("", "/resource"); }));
      Check(rejects_request([&] { custom.ToString("local\thost", "/resource"); }));
      Check(rejects_request([&] { custom.ToString("localhost", "/resource\r\nInjected: y"); }));
      custom.method = "GET\r\nInjected";
      Check(rejects_request([&] { custom.ToString("localhost", "/resource"); }));
      custom.method = "PUT";
      const auto location = http::ExtractLocationHeader(
          "HTTP/1.1 302 Found\r\n"
          "lOcAtIoN:\thttps://example.test/next\r\n\r\n");
      Check(location && *location == "https://example.test/next");
      const auto trimmed_location = http::ExtractLocationHeader(
          "HTTP/1.1 302 Found\r\n"
          "Location: https://example.test/next \t\r\n\r\n");
      Check(trimmed_location && *trimmed_location == "https://example.test/next");
      Check(!http::ExtractLocationHeader(
          "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"));
      Check(!http::ExtractLocationHeader(
          "HTTP/1.1 302 Found\r\nLocation: \r\n\r\n"));
      Check(!http::ExtractLocationHeader(
          "HTTP/1.1 302 Found\nLocation: https://example.test/next\n\n"));
      Check(!http::ExtractLocationHeader(
          "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"
          "body\r\nLocation: https://example.test/forged\r\n"));
      Check(!http::ExtractLocationHeader(
          "HTTP/1.1 302 Found\r\nLocation: https://example.test/partial"));
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
                             std::to_string(ntohs(address.sin_port)), false) == 0);
        Fd peer{accept(listener.value, nullptr, nullptr)};
        Check(peer.value >= 0);
        Check(client.Send("ping") == 4);
        char buffer[4];
        Check(recv(peer.value, buffer, 4, MSG_WAITALL) == 4);
        Check(std::string(buffer, 4) == "ping");
        Check(send(peer.value, "pong", 4, 0) == 4);
        Check(shutdown(peer.value, SHUT_WR) == 0);
        Check(client.Receive(0, 0).empty());
        Check(client.Receive(MSG_WAITALL) == "pong");
      }
      Check(Fds() == count);
      {
        Fd listener{socket(AF_INET, SOCK_STREAM, 0)};
        Check(listener.value >= 0);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Check(bind(listener.value, reinterpret_cast<sockaddr *>(&address),
                   sizeof(address)) == 0);
        Check(listen(listener.value, 1) == 0);
        socklen_t length = sizeof(address);
        Check(getsockname(listener.value,
                          reinterpret_cast<sockaddr *>(&address),
                          &length) == 0);
        Socket client;
        client.SetIoTimeout(150);
        Socket moved(std::move(client));
        Check(!client.isValid());
        Check(moved.Connect("127.0.0.1",
                             std::to_string(ntohs(address.sin_port)), false) == 0);
        Fd peer{accept(listener.value, nullptr, nullptr)};
        Check(peer.value >= 0);
        auto receive_timeout = [](Socket &socket) {
          const auto start = std::chrono::steady_clock::now();
          Check(socket.Receive().empty());
          Check(errno == EAGAIN || errno == EWOULDBLOCK);
          return std::chrono::steady_clock::now() - start;
        };
        const auto before_connect = receive_timeout(moved);
        Check(before_connect >= std::chrono::milliseconds(75));
        Check(before_connect < std::chrono::seconds(2));
        Socket assigned;
        assigned = std::move(moved);
        Check(!moved.isValid());
        assigned.SetIoTimeout(75);
        const auto after_connect = receive_timeout(assigned);
        Check(after_connect >= std::chrono::milliseconds(35));
        Check(after_connect < std::chrono::seconds(2));
        bool rejected_negative = false;
        try {
          assigned.SetIoTimeout(-1);
        } catch (const std::invalid_argument &) {
          rejected_negative = true;
        }
        Check(rejected_negative);
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
