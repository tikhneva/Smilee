#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <string>
#include <chrono>
#include <cstdlib> 
#include <thread>
#include <conio.h> 
using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

SOCKET ClientSocket = INVALID_SOCKET;
SOCKET ListenSocket = INVALID_SOCKET;

COORD client_smile, server_smile;

unsigned int client_coins = 0, server_coins = 0;

bool g_stopThreads = false;


DWORD WINAPI TimerThread(LPVOID param);
bool IsCoinCollected(char** map, unsigned int rows, unsigned int cols, int direction);
void BuildMap(char** map, unsigned int height, unsigned int width);
void DisplayMap(char** map, unsigned int height, unsigned int width);
bool CanSmileMove(char** map, unsigned int rows, unsigned int cols, int direction);
DWORD WINAPI SenderThread(void* param);
DWORD WINAPI ReceiverThread(void* param);



int main() {
    setlocale(0, "");
    srand(time(0));
    system("title СЕРВЕР");

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cout << "WSAStartup завершился с ошибкой: " << iResult << "\n";
        cout << "подключение Winsock.dll прошло с ошибкой!\n";
        return 1;
    }

    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* result = NULL;
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        cout << "getaddrinfo завершился с ошибкой: " << iResult << "\n";
        cout << "получение адреса и порта сервера прошло c ошибкой!\n";
        WSACleanup();
        return 2;
    }

    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        cout << "socket завершился с ошибкой: " << WSAGetLastError() << "\n";
        cout << "создание сокета прошло c ошибкой!\n";
        freeaddrinfo(result);
        WSACleanup();
        return 3;
    }

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        cout << "bind завершился с ошибкой: " << WSAGetLastError() << "\n";
        cout << "внедрение сокета по IP-адресу прошло с ошибкой!\n";
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 4;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        cout << "listen завершился с ошибкой: " << WSAGetLastError() << "\n";
        cout << "прослушивание информации от клиента не началось. что-то пошло не так!\n";
        closesocket(ListenSocket);
        WSACleanup();
        return 5;
    }

    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        cout << "accept завершился с ошибкой: " << WSAGetLastError() << "\n";
        cout << "соединение с клиентским приложением не установлено! печаль!\n";
        closesocket(ListenSocket);
        WSACleanup();
        return 6;
    }


    HANDLE timeCounterHandle = CreateThread(0, 0, TimerThread, 0, 0, 0);

    HANDLE senderHandle = CreateThread(0, 0, SenderThread, 0, 0, 0);
    HANDLE receiverHandle = CreateThread(0, 0, ReceiverThread, 0, 0, 0);

    WaitForSingleObject(receiverHandle, INFINITE);
    WaitForSingleObject(senderHandle, INFINITE);
    WaitForSingleObject(timeCounterHandle, INFINITE);


    CloseHandle(receiverHandle);
    CloseHandle(senderHandle);
    CloseHandle(timeCounterHandle);

    system("cls");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);

    char clientCoinsStr[200];
    string servCoinsStr = to_string(server_coins);
    iResult = recv(ClientSocket, clientCoinsStr, strlen(clientCoinsStr), 0);
    clientCoinsStr[iResult] = '\0';
    client_coins = stoi(clientCoinsStr);

    iResult = send(ClientSocket, servCoinsStr.c_str(), servCoinsStr.size(), 0);
    server_coins = stoi(servCoinsStr);

    cout << "Server collected coins: " << server_coins << endl;
    cout << "Client collected coins: " << client_coins << endl;

    return 0;
}







DWORD WINAPI TimerThread(LPVOID param) {
    auto start = chrono::steady_clock::now();

    while (true) {
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(now - start).count();

        int hours = elapsed / 3600;
        int minutes = (elapsed % 3600) / 60;
        int seconds = elapsed % 60;

        char timeStr[9];
        sprintf_s(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);

        SetConsoleTitleA(timeStr);

        this_thread::sleep_for(chrono::milliseconds(100));

        if (elapsed >= 20) {
            g_stopThreads = true; 
            break; 
        }
    }


    return 0;
}



bool IsCoinCollected(char** map, unsigned int rows, unsigned int cols, int direction) {
    if (direction == 1) {
        if (server_smile.X - 1 > 0 && map[server_smile.Y][server_smile.X - 1] == '*')
            return true;
        else
            return false;
    }
    else if (direction == 2) {
        if (server_smile.X + 1 < cols - 1 && map[server_smile.Y][server_smile.X + 1] == '*')
            return true;
        else
            return false;
    }
    else if (direction == 3) {
        if (server_smile.Y - 1 > 0 && map[server_smile.Y - 1][server_smile.X] == '*')
            return true;
        else
            return false;
    }
    else if (direction == 4) {
        if (server_smile.Y + 1 < rows - 1 && map[server_smile.Y + 1][server_smile.X] == '*')
            return true;
        else
            return false;
    }
    return false;
}

void BuildMap(char** map, unsigned int height, unsigned int width) {
    srand(time(0));
    const int walls = 20;
    const int treasures = 15;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            map[i][j] = ' ';
        }
    }

    for (int i = 0; i < height; i++) {
        map[i][0] = '|';
        map[i][width - 1] = '|';
    }
    for (int j = 0; j < width; j++) {
        map[0][j] = '|';
        map[height - 1][j] = '|';
    }

    for (int x = 0; x < walls; x++) {
        int coord_x = 1 + rand() % (width - 2); // Avoiding the border
        int coord_y = 1 + rand() % (height - 2); // Avoiding the border
        map[coord_y][coord_x] = '|';
    }

    for (int x = 0; x < treasures; x++) {
        int coord_x, coord_y;
        do {
            coord_x = 1 + rand() % (width - 2); // Avoiding the border
            coord_y = 1 + rand() % (height - 2); // Avoiding the border
        } while (map[coord_y][coord_x] != ' '); // Ensure it's not already occupied
        map[coord_y][coord_x] = '*';
    }

    map[1][1] = '^'; // Server starting position
    map[height - 2][1] = '#'; // Client starting position
}



void DisplayMap(char** map, unsigned int height, unsigned int width) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (map[i][j] == ' ') {
                SetConsoleTextAttribute(h, 0);
                cout << ' ';
            }
            else if (map[i][j] == '|') {
                SetConsoleTextAttribute(h, 4);
                cout << '|';
            }
            else if (map[i][j] == '^') {
                SetConsoleTextAttribute(h, 2);
                server_smile.X = j;
                server_smile.Y = i;
                cout << '^'; //server
            }
            else if (map[i][j] == '#') {
                client_smile.X = j;
                client_smile.Y = i;
                SetConsoleTextAttribute(h, 9);
                cout << '#'; //client
            }
            else if (map[i][j] == '*')
            {
                SetConsoleTextAttribute(h, 14);
                cout << '*';
            }
        }
        cout << endl;
    }

}


bool CanSmileMove(char** map, unsigned int rows, unsigned int cols, int direction) {
    if (direction == 1) {
        if (server_smile.X - 1 > 0 && map[server_smile.Y][server_smile.X - 1] != '|')
            return true;
        else
            return false;
    }
    else if (direction == 2) {
        if (server_smile.X + 1 < cols - 1 && map[server_smile.Y][server_smile.X + 1] != '|')
            return true;
        else
            return false;
    }
    else if (direction == 3) {
        if (server_smile.Y - 1 > 0 && map[server_smile.Y - 1][server_smile.X] != '|')
            return true;
        else
            return false;
    }
    else if (direction == 4) {
        if (server_smile.Y + 1 < rows - 1 && map[server_smile.Y + 1][server_smile.X] != '|')
            return true;
        else
            return false;
    }
    return false;
}

string MakeMessage(char** map, unsigned int height, unsigned int width) {
    string message;
    message = "h" + to_string(height) + "w" + to_string(width) + "d";
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            message += map[i][j];
        }
    }
    return message;
}
DWORD WINAPI SenderThread(void* param) {
    unsigned int height = 15, width = 25;
    char** map = new char* [height];
    for (int i = 0; i < height; i++) {
        map[i] = new char[width];
    }
    BuildMap(map, height, width);
    DisplayMap(map, height, width);
    string message = MakeMessage(map, height, width);

    int iSend = send(ClientSocket, message.c_str(), message.size(), 0);

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    while (!g_stopThreads) {
        if (g_stopThreads)
            TerminateThread(h, 0);
        int code = _getch();
        int direction = 0;

        if (code == 224 || code == 0) code = _getch();

        if (code == 75) {//left
            direction = 1;
        }
        else if (code == 77) {//right
            direction = 2;
        }
        else if (code == 72) {//up
            direction = 3;
        }
        else if (code == 80) {//down
            direction = 4;
        }

        if (CanSmileMove(map, height, width, direction)) {
            if (IsCoinCollected(map, height, width, direction)) {
                server_coins++;
            }
            SetConsoleCursorPosition(h, server_smile);
            cout << ' ';

            if (code == 75) {//left
                server_smile.X--;
            }
            else if (code == 77) {//right
                server_smile.X++;
            }
            else if (code == 72) {//up
                server_smile.Y--;
            }
            else if (code == 80) {//down
                server_smile.Y++;
            }

            SetConsoleCursorPosition(h, server_smile);
            SetConsoleTextAttribute(h, 2);
            cout << '^';

            if (server_smile.X == client_smile.X && server_smile.Y == client_smile.Y) {
                system("title INTERSECTED!");
            }

            char message[200];
            strcpy_s(message, to_string(direction).c_str());
            send(ClientSocket, message, strlen(message), 0);
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // Очистка памяти и закрытие сокета
    for (int i = 0; i < height; i++) {
        delete[] map[i];
    }
    delete[] map;
    return 0;
}


DWORD WINAPI ReceiverThread(void* param) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    while (!g_stopThreads) {
        if (!g_stopThreads)
            TerminateThread(h, 0);
        char message[200];
        int iResult = recv(ClientSocket, message, strlen(message), 0);
        message[iResult] = '\0';
        string strMessage = message;

        SetConsoleCursorPosition(h, client_smile);
        SetConsoleTextAttribute(h, 0);
        cout << ' ';
        if (iResult > 0) {
            if (strMessage == "1")
                client_smile.X--;
            else if (strMessage == "2")
                client_smile.X++;
            else if (strMessage == "3")
                client_smile.Y--;
            else if (strMessage == "4")
                client_smile.Y++;

            SetConsoleCursorPosition(h, client_smile);
            SetConsoleTextAttribute(h, 9);
            cout << '$';

            if (server_smile.X == client_smile.X && server_smile.Y == client_smile.Y) {
                system("title INTERSECTED!");
            }
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    return 0;
}
