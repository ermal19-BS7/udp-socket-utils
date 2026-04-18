#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <WS2tcpip.h>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

using namespace std;
namespace fs = std::filesystem;

// CONFIG
string SERVER_IP = "0.0.0.0";
int SERVER_PORT = 54000;
int HTTP_PORT = 8080;
int MAX_CLIENTS = 4;
int TIMEOUT_SEC = 30;

// GLOBALS
unordered_map<string, string> clients;
unordered_map<string, chrono::steady_clock::time_point> lastSeen;
vector<string> messageLog;
mutex mtx;

// ================= LOG FUNCTION =================
void logMsg(const string& msg)
{
    cout << msg << endl;
}

// ================= UTIL =================
string getClientKey(sockaddr_in& client)
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, ip, INET_ADDRSTRLEN);
    return string(ip) + ":" + to_string(ntohs(client.sin_port));
}

// ================= TIMEOUT =================
void cleanupClients()
{
    while (true)
    {
        this_thread::sleep_for(chrono::seconds(5));
        lock_guard<mutex> lock(mtx);

        auto now = chrono::steady_clock::now();

        for (auto it = lastSeen.begin(); it != lastSeen.end();)
        {
            if (chrono::duration_cast<chrono::seconds>(now - it->second).count() > TIMEOUT_SEC)
            {
                logMsg("[TIMEOUT] Client removed: " + it->first);

                clients.erase(it->first);
                it = lastSeen.erase(it);
            }
            else ++it;
        }
    }
}

// ================= HTTP SERVER =================
void httpServer()
{
    SOCKET httpSock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in hint{};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(HTTP_PORT);
    hint.sin_addr.s_addr = INADDR_ANY;

    bind(httpSock, (sockaddr*)&hint, sizeof(hint));
    listen(httpSock, SOMAXCONN);

    logMsg("[HTTP] Server started on port 8080");

    while (true)
    {
        SOCKET client = accept(httpSock, nullptr, nullptr);

        logMsg("[HTTP] Request received");

        char buf[4096]{};
        recv(client, buf, sizeof(buf), 0);

        string response;

        if (string(buf).find("GET /stats") != string::npos)
        {
            lock_guard<mutex> lock(mtx);

            stringstream ss;
            ss << "{\n";
            ss << "  \"clients\": " << clients.size() << ",\n";

            ss << "  \"ips\": [";
            for (auto& c : clients)
                ss << "\"" << c.first << "\",";
            ss << "],\n";

            ss << "  \"messages_count\": " << messageLog.size() << "\n";
            ss << "}\n";

            response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + ss.str();
        }
        else
        {
            response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }

        send(client, response.c_str(), response.size(), 0);
        closesocket(client);
    }
}

// ================= MAIN =================
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

    logMsg("[SERVER] Started on port 54000");

    thread(cleanupClients).detach();
    thread(httpServer).detach();

    sockaddr_in client{};
    int clientLength = sizeof(client);
    char buf[8192];
