#include "socket.h"
#include "httpresponse.h"
#include <utility>
#include <vector>
#include <iostream>
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
        std::cout << "Received response from www.google.com:\n" << response.substr(0, 100) << "...\n" << std::endl;

        HttpResponse parsedResponse = HttpResponse::Parse(response);
        std::string location = parsedResponse.headers["location"];
        std::cout << "Parsed status code: " << parsedResponse.statusCode << std::endl;
        std::cout << "Extracted Location header: " << location << std::endl;

        if (!location.empty()) {
            Socket redirectSocket;
            size_t hostStart = location.find("://");
            if (hostStart != std::string::npos) {
                hostStart += 3;
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
                    std::cout << "Received response from redirected location:\n" << redirectResponse.substr(0, 100) << "...\n" << std::endl;
                }
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
