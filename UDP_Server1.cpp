#include <iostream>
#include <WS2tcpip.h>
#include <sstream>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <filesystem>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int SERVER_PORT = 54000;

unordered_map<string, chrono::steady_clock::time_point> lastSeen;

void cleanupClients()
{
    while (true)
    {
        this_thread::sleep_for(chrono::seconds(5));
    }
}

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

        string cmd(buf, bytesIn);
        namespace fs = std::filesystem;

        if (cmd == "/list")
        {
            for (auto& f : fs::directory_iterator("."))
                response += f.path().filename().string() + "\n";
        }
        else if (cmd.rfind("/read ", 0) == 0)
        {
            ifstream f(cmd.substr(6));
            string line;
            while (getline(f, line))
                response += line + "\n";
        }
        else if (cmd.rfind("/search ", 0) == 0)
        {
            string keyword = cmd.substr(8);
            for (auto& f : fs::directory_iterator("."))
            {
                string name = f.path().filename().string();
                if (name.find(keyword) != string::npos)
                    response += name + "\n";
            }
        }
        else if (cmd.rfind("/delete ", 0) == 0)
        {
           
            if (clients[key] != "admin")
                response = "Permission denied";
            else
            {
                fs::remove(cmd.substr(8));
                response = "Deleted";
            }
        }

        cout << "Received: " << string(buf, bytesIn) << endl;

        sendto(in, buf, bytesIn, 0,
            (sockaddr*)&client, clientLength);
    }

}

