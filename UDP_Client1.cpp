#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

string sendCommand(SOCKET sock, sockaddr_in& server, string cmd)
{
    sendto(sock, cmd.c_str(), cmd.size() + 1, 0,
        (sockaddr*)&server, sizeof(server));

    char buf[8192];
    int len = sizeof(server);

    int bytes = recvfrom(sock, buf, 8192, 0,
        (sockaddr*)&server, &len);

    return string(buf, bytes);
