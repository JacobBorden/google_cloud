#include "socket.h"
#include <utility>
#include <vector>
#include <iostream>
#include <regex>
SSL_CTX* Socket::ssl_ctx = nullptr;

inline std::string ExtractHeader(const std::string& response, const std::string& headerName) {
    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd == std::string::npos) headerEnd = response.length();
    std::string headers = response.substr(0, headerEnd);

    std::regex headerRegex("(?:\r\n|^)" + headerName + ":\\s*([^\r\n]+)", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(headers, match, headerRegex)) {
        std::string value = match[1].str();
        size_t last = value.find_last_not_of(" \t");
        if (last != std::string::npos) value.erase(last + 1);
        else value.clear();
        return value;
    }
    return "";
}
int main()
{
    {
    std::vector<Socket> sockets;
    sockets.emplace_back(); // Create and add a Socket instance to the vector
        Socket anotherSocket = std::move(sockets.back()); // Move the last Socket to anotherSocket
    if(anotherSocket.isValid()) {
        // Use anotherSocket
        std::cout << "Socket moved successfully." << std::endl;
    }
   
    Socket googleSocket;
    if (googleSocket.Connect("www.google.com", "80", false) == 0){
        std::string httpRequest = "GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: close\r\n\r\n";
        googleSocket.Send(httpRequest);
        std::string response;
        std::string chunk;
        do {
           chunk = googleSocket.Receive();
            response += chunk;
        } while (chunk.length() > 0);
        std::cout << "Received response from www.google.com:\n" << response << std::endl;

        std::string location = ExtractHeader(response, "Location");
        std::cout << "Extracted Location header: " << location << std::endl;
        if (!location.empty()) {
            Socket redirectSocket;
            size_t hostStart = location.find("://") + 3;
            size_t hostEnd = location.find("/", hostStart);
            std::string locationHost = location.substr(hostStart, hostEnd - hostStart);
            if (redirectSocket.Connect(locationHost, "443", true) == 0)
            {
                std::string redirectRequest = "GET / HTTP/1.1\r\nHost: " + locationHost + "\r\nConnection: close\r\n\r\n";
                redirectSocket.Send(redirectRequest);
                std::string redirectResponse;
                std::string redirectChunk;
                do {
                    redirectChunk = redirectSocket.Receive();
                    redirectResponse += redirectChunk;
                } while (redirectChunk.length() > 0);
                std::cout << "Received response from redirected location:\n" << redirectResponse << std::endl;
            }
        }
    }

    }

    if (Socket::ssl_ctx != nullptr) {
        SSL_CTX_free(Socket::ssl_ctx);
        Socket::ssl_ctx = nullptr;
    }

    return 0;
}
