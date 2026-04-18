#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

string SERVER_IP = "127.0.0.1";
int SERVER_PORT = 54000;

string sendCommand(SOCKET sock, sockaddr_in& server, string cmd)
{
    sendto(sock, cmd.c_str(), cmd.size() + 1, 0,
        (sockaddr*)&server, sizeof(server));

    char buf[8192];
    int serverLen = sizeof(server);

    int bytesReceived = recvfrom(sock, buf, 8192, 0,
        (sockaddr*)&server, &serverLen);

    if (bytesReceived == SOCKET_ERROR)
        return "No response";

    return string(buf);
}

int main()
{
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &server.sin_addr);

    string user, pass;
    cout << "Username: ";
    cin >> user;
    cout << "Password: ";
    cin >> pass;
    cin.ignore();

    string res = sendCommand(sock, server, "/login " + user + " " + pass);
    cout << res << endl;

    bool isAdmin = (user == "admin");