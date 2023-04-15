#include <iostream>
#include <string>
#include <ws2tcpip.h>
#include <winsock2.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

struct User {
    std::string USER;
    std::string ACTION;
};

int initializeWinsock();
SOCKET createSocket();
sockaddr_in createServerAddress();
int connectToServer(SOCKET sock, sockaddr_in serverAddr);
std::string createHttpGetRequest(const User& user);
int sendRequest(SOCKET sock, std::string request);
std::pair<std::string, std::string> receiveResponse(SOCKET sock);
std::string parseTokenFromResponse(std::string response);




int main() {

    int iResult = initializeWinsock();
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    SOCKET sock = createSocket();
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr = createServerAddress();

    iResult = connectToServer(sock, serverAddr);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    User user;
    user.USER = "michal";
    user.ACTION = "UPLOAD";
    std::string request = createHttpGetRequest(user);

    iResult = sendRequest(sock, request);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "send failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::pair<std::string, std::string> response = receiveResponse(sock);
    if (response.first == "ERROR") {
        std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "HTTP header: " << response.first << std::endl;

    std::string token = parseTokenFromResponse(response.second);
    if (token.empty()) {
        std::cout << "TOKEN not found in response" << std::endl;
    }
    else {
        std::cout << "TOKEN: " << token << std::endl;
    }


    return 0;
}

int initializeWinsock() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

SOCKET createSocket() {
    return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

sockaddr_in createServerAddress() {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080); // port 8080
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr.s_addr);

    return serverAddr;
}

int connectToServer(SOCKET sock, sockaddr_in serverAddr) {
    return connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
}

std::string createHttpGetRequest(const User& user) {
    std::string request = "GET /getStatus?USER=" + user.USER + "&ACTION=" + user.ACTION + " HTTP/1.0\r\n";
    request += "Host: localhost:8080\r\nConnection: close\r\n\r\n";
    return request;
}

std::pair<std::string, std::string> receiveResponse(SOCKET sock) {
    std::string response;
    char buffer[2048];
    while (true) {
        int iResult = recv(sock, buffer, 2048, 0);
        if (iResult > 0) {
            response.append(buffer, iResult);
        }
        else if (iResult == 0) {
            std::cout << "Received empty response" << std::endl;
            break;
        }
        else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
        }
    }

    size_t pos = response.find("\r\n\r\n");
    if (pos == std::string::npos) {
        std::cerr << "Failed to separate HTTP header from data field" << std::endl;
        return {};
    }
    std::string header = response.substr(0, pos);
    std::string data = response.substr(pos + 4);

    return { header, data };
}

std::string parseTokenFromResponse(std::string response) {
    std::string token;
    size_t pos = response.find("TOKEN"); 
    if (pos != std::string::npos) {
        size_t endPos = response.find("</body>", pos); 
        if (endPos != std::string::npos) {
            token = response.substr(pos + 10, endPos - pos - 14);
        }
    }

    return token;
}

int sendRequest(SOCKET sock, std::string request) {
    return send(sock, request.c_str(), request.length(), 0);
}