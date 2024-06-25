#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include <vector>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

atomic_bool stopThread(false);

// проверка отправки сообщения 
void checkSend(int sendResult) {
    if (sendResult == SOCKET_ERROR)
    {
        cerr << "Can't send message to server" << endl;
    }
}

class Client {
private:
    int reconectCount = 0;
    SOCKET sock;
    string name;
    thread receiveThread;
    string ipAddress;
    int port;

public:
    Client(const string& ip, int p) : ipAddress(ip), port(p) {}

    ~Client() {
        Disconnect();
    }

    void Start() {
        Connect();

        string userInput;

        cout << "ENTER your name, please: > ";
        getline(cin, name);

        receiveThread = thread(&Client::ReceiveThread, this);

        do
        {
            cout << "> ";
            getline(cin, userInput);

            int sendResult = Send(userInput);
            checkSend(sendResult);

        } while (userInput.size() > 0 && !stopThread);

        stopThread = true;
        receiveThread.join();
    }

private:
    void Connect() {
        while (!stopThread) {
            WSAData data;
            WORD ver = MAKEWORD(2, 2);
            int wsResult = WSAStartup(ver, &data);
            if (wsResult != 0)
            {
                cerr << "Can't start winsock, Err #" << wsResult << endl;
                continue;
            }

            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == INVALID_SOCKET)
            {
                cerr << "Can't create socket, Err #" << WSAGetLastError() << endl;
                WSACleanup();
                continue;
            }

            sockaddr_in hint;
            hint.sin_family = AF_INET;
            hint.sin_port = htons(port);
            inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

            int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
            if (connResult == SOCKET_ERROR)
            {
                cerr << "Can't connect to server" << endl;
                closesocket(sock);
                WSACleanup();
                continue;
            }

            return;
        }
    }

    void Disconnect() {
        //closesocket(sock);
        WSACleanup();
    }

    void Reconnect() {
        Disconnect();
        reconectCount += 1;

        if (reconectCount >= 20000000000000000)
        {
            exit(1);
        }

        this_thread::sleep_for(chrono::seconds(2));
        Connect();

        
    }

    void ReceiveThread() {
        char buf[4096];
        while (!stopThread)
        {
            ZeroMemory(buf, 4096);
            int bytesReceived = recv(sock, buf, 4096, 0);
            if (bytesReceived > 0)
            {
                string message = buf;
                cout << "SERVER> " << message << endl;
            }
            else if (bytesReceived == 0) {
                cerr << "Disconnected from the server. Reconnecting..." << endl;
                Reconnect();
            }
            else {
                cerr << "Error in receiving data from the server. Reconnecting..." << endl;
                Reconnect();
            }
        }
    }

    int Send(const string& message) {
        return send(sock, (message + "\n").c_str(), message.size() + 1, 0);
    }
};

int main()
{
    // !!!it is important to change the port and ipadress to exactly the same as on the server!!!
    string ipAddress = "192.168.1.75"; // enter ip address of the server
    int port = 54000;               // default port !!!(change it if you are busy)

    Client client(ipAddress, port);
    client.Start();


    return 0;
}
