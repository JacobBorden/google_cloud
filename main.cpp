#include "socket.h"
#include <utility>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cctype>

std::string ExtractHeader(const std::string& response, const std::string& header_name) {
    size_t pos = 0;
    std::string lower_header = header_name;
    std::transform(lower_header.begin(), lower_header.end(), lower_header.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    while (pos < response.length()) {
        size_t endline = response.find("\r\n", pos);
        if (endline == std::string::npos) endline = response.length();

        if (endline == pos) break; // Empty line implies end of headers

        size_t colon = response.find(':', pos);
        if (colon != std::string::npos && colon < endline) {
            std::string name = response.substr(pos, colon - pos);

            size_t nameEnd = name.length();
            while (nameEnd > 0 && (name[nameEnd-1] == ' ' || name[nameEnd-1] == '\t')) nameEnd--;
            name = name.substr(0, nameEnd);

            std::string lower_name = name;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                           [](unsigned char c){ return std::tolower(c); });

            if (lower_name == lower_header) {
                size_t valStart = colon + 1;
                while (valStart < endline && (response[valStart] == ' ' || response[valStart] == '\t')) {
                    valStart++;
                }
                size_t valEnd = endline;
                while (valEnd > valStart && (response[valEnd-1] == ' ' || response[valEnd-1] == '\t')) {
                    valEnd--;
                }
                return response.substr(valStart, valEnd - valStart);
            }
        }

        if (endline == response.length()) break;
        pos = endline + 2;
    }
    return "";
}

SSL_CTX* Socket::ssl_ctx = nullptr;
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
