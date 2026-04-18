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
  while (true)
    {
        ZeroMemory(&client, clientLength);
        ZeroMemory(buf, sizeof(buf));

        int bytesIn = recvfrom(in, buf, sizeof(buf), 0, (sockaddr*)&client, &clientLength);
        if (bytesIn == SOCKET_ERROR) continue;

        string cmd(buf);
        string key = getClientKey(client);
        string response;

        logMsg("[CMD] " + key + " -> " + cmd);

        {
            lock_guard<mutex> lock(mtx);

            if (clients.size() >= MAX_CLIENTS && clients.find(key) == clients.end())
            {
                response = "Server full";
                sendto(in, response.c_str(), response.size() + 1, 0, (sockaddr*)&client, clientLength);
                logMsg("[REJECTED] Server full: " + key);
                continue;
            }

            lastSeen[key] = chrono::steady_clock::now();
            messageLog.push_back(cmd);
        }

        // LOGIN
        if (cmd.rfind("/login ", 0) == 0)
        {
            istringstream iss(cmd);
            string tmp, user, pass;
            iss >> tmp >> user >> pass;

            lock_guard<mutex> lock(mtx);

            if (user == "admin" && pass == "1234")
            {
                clients[key] = "admin";
                response = "Logged in as ADMIN";

                logMsg("[LOGIN] " + key + " -> ADMIN");
            }
            else if (user == "user" && pass == "1111")
            {
                clients[key] = "user";
                response = "Logged in as USER";

                logMsg("[LOGIN] " + key + " -> USER");
            }
            else
            {
                response = "Login failed";
                logMsg("[LOGIN FAILED] " + key);
            }
        }
        else
        {
            lock_guard<mutex> lock(mtx);

            if (clients.find(key) == clients.end())
            {
                response = "Please login first";
                logMsg("[UNAUTH] " + key);
            }
            else
            {
                string role = clients[key];

                if (role == "user")
                    this_thread::sleep_for(chrono::milliseconds(200));

                if (cmd == "/list")
                {
                    for (auto& f : fs::directory_iterator("."))
                        response += f.path().filename().string() + "\n";
                }
                else if (cmd.rfind("/read ", 0) == 0)
                {
                    string fileName = cmd.substr(6);

                    cout << "[READ] file requested: " << fileName << endl;

                    ifstream f(fileName);

                    if (!f.is_open())
                    {
                        response = "File not found";
                        cout << "[READ] FAILED opening file" << endl;
                    }
                    else
                    {
                        string line;
                        response.clear();

                        while (getline(f, line))
                        {
                            response += line + "\n";
                        }

                        cout << "[READ] SUCCESS" << endl;
                    }
                }
                else if (cmd.rfind("/upload ", 0) == 0)
                {
                    if (role != "admin") response = "Permission denied";
                    else
                    {
                        size_t sep = cmd.find('|');
                        string filename = cmd.substr(8, sep - 8);
                        string content = cmd.substr(sep + 1);

                        ofstream out(filename);
                        out << content;
                        response = "Uploaded";
                    }
                }
                else if (cmd.rfind("/download ", 0) == 0)
                {
                    ifstream f(cmd.substr(10));
                    if (!f) response = "File not found";
                    else
                    {
                        stringstream ss;
                        ss << f.rdbuf();
                        response = ss.str();
                    }
                }
                else if (cmd.rfind("/delete ", 0) == 0)
                {
                    if (role != "admin") response = "Permission denied";
                    else
                        response = fs::remove(cmd.substr(8)) ? "Deleted" : "Error";
                }
                else if (cmd.rfind("/search ", 0) == 0)
                {
                    string keyw = cmd.substr(8);
                    for (auto& f : fs::directory_iterator("."))
                        if (f.path().filename().string().find(keyw) != string::npos)
                            response += f.path().filename().string() + "\n";
                }
                else if (cmd.rfind("/info ", 0) == 0)
                {
                    string file = cmd.substr(6);
                    if (fs::exists(file))
                        response = "Size: " + to_string(fs::file_size(file));
                    else
                        response = "File not found";
                }
                else
                {
                    response = "Unknown command";
                }
            }
        }

        logMsg("[RESPONSE] " + key + " -> " + response);

        sendto(in, response.c_str(), response.size() + 1, 0,
            (sockaddr*)&client, clientLength);
    }
}
