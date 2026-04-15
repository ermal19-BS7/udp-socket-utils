#include <iostream>
#include <WS2tcpip.h>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

int SERVER_PORT = 54000;

int main()
{
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);

    SOCKET in = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in serverHint{};
    serverHint.sin_family = AF_INET;
    serverHint.sin_port = htons(SERVER_PORT);
    serverHint.sin_addr.s_addr = INADDR_ANY;

    bind(in, (sockaddr*)&serverHint, sizeof(serverHint));

    sockaddr_in client{};
    int clientLength = sizeof(client);
    char buf[4096];

    stringstream ss;
ss << "{ \"clients\": " << clients.size() << " }";
string response = ss.str();

    while (true)
    {
        int bytesIn = recvfrom(in, buf, sizeof(buf), 0,
            (sockaddr*)&client, &clientLength);

        if (bytesIn == SOCKET_ERROR) continue;

        cout << "Received: " << string(buf, bytesIn) << endl;

        sendto(in, buf, bytesIn, 0,
            (sockaddr*)&client, clientLength);
    }

}

