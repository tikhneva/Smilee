#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <cstdlib> 
#include <string>
#include <chrono>
#include <thread>
#include <conio.h> 

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

char** map_array = nullptr; 
unsigned int rows=0, cols=0;

COORD client_smile, server_smile;
SOCKET ConnectSocket = INVALID_SOCKET;

unsigned int client_coins = 0, server_coins = 0;

bool g_stopThreads = false;


DWORD WINAPI TimerThread(LPVOID param);
bool CanSmileMove(char** map_array, unsigned int rows, unsigned int cols, int direction);
bool IsCoinCollected(char** map_array, unsigned int rows, unsigned int cols, int direction);
void ShowMap(char** map_array, unsigned int height, unsigned int width);
void ParseData(char message[], char**& map_array, unsigned int& rows, unsigned int& cols);
DWORD WINAPI Receiver(LPVOID param);
DWORD WINAPI Sender(void* param);




int main() {
	setlocale(0, "");
	system("title КЛИЕНТ");

	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup завершился с ошибкой: " << iResult << "\n";
		return 11;
	}

	addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	const char* ip = "localhost";
	addrinfo* result = NULL;
	iResult = getaddrinfo(ip, DEFAULT_PORT, &hints, &result);

	if (iResult != 0) {
		cout << "getaddrinfo завершился с ошибкой: " << iResult << "\n";
		WSACleanup();
		return 12;
	}

	for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (ConnectSocket == INVALID_SOCKET) {
			cout << "socket завершился с ошибкой: " << WSAGetLastError() << "\n";
			WSACleanup();
			return 13;
		}

		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}

		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		cout << "невозможно подключиться к серверу!\n";
		WSACleanup();
		return 14;
	}

	// Создаем потоки с использованием CreateThread
	HANDLE receiverThreadHandle = CreateThread(0, 0, Receiver, 0, 0, 0);
	HANDLE senderThreadHandle = CreateThread(0, 0, Sender, 0, 0, 0);
	HANDLE timeThread = CreateThread(0, 0, TimerThread, 0, 0, 0);
	if (receiverThreadHandle == NULL) {
		cout << "Ошибка создания потоков.\n";
		closesocket(ConnectSocket);
		WSACleanup();
		return 16;
	}



	// Ожидаем завершения потоков
	WaitForSingleObject(receiverThreadHandle, INFINITE);
	WaitForSingleObject(senderThreadHandle, INFINITE);
	WaitForSingleObject(timeThread, INFINITE);


	CloseHandle(receiverThreadHandle);
	CloseHandle(senderThreadHandle);
	CloseHandle(timeThread);



	system("cls");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
	char servCoinsStr[200];
	string clientCoinsStr = to_string(client_coins);
	iResult = send(ConnectSocket, clientCoinsStr.c_str(), clientCoinsStr.size(), 0);

	iResult = recv(ConnectSocket, servCoinsStr, strlen(servCoinsStr), 0);
	servCoinsStr[iResult] = '\0';

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





	bool CanSmileMove(char** map_array, unsigned int rows, unsigned int cols, int direction) {
		if (direction == 1) {
			if (client_smile.X - 1 > 0 && map_array[client_smile.Y][client_smile.X - 1] != '|')
				return true;
			else
				return false;
		}
		else if (direction == 2) {
			if (client_smile.X + 1 < cols - 1 && map_array[client_smile.Y][client_smile.X + 1] != '|')
				return true;
			else
				return false;
		}
		else if (direction == 3) {
			if (client_smile.Y - 1 > 0 && map_array[client_smile.Y - 1][client_smile.X] != '|')
				return true;
			else
				return false;
		}
		else if (direction == 4) {
			if (client_smile.Y + 1 < rows - 1 && map_array[client_smile.Y + 1][client_smile.X] != '|')
				return true;
			else
				return false;
		}
		return false;
	}

	bool IsCoinCollected(char** map_array, unsigned int rows, unsigned int cols, int direction) {
		if (direction == 1) {
			if (client_smile.X - 1 > 0 && map_array[client_smile.Y][client_smile.X - 1] == '*')
				return true;
			else
				return false;
		}
		else if (direction == 2) {
			if (client_smile.X + 1 < cols - 1 && map_array[client_smile.Y][client_smile.X + 1] == '*')
				return true;
			else
				return false;
		}
		else if (direction == 3) {
			if (client_smile.Y - 1 > 0 && map_array[client_smile.Y - 1][client_smile.X] == '*')
				return true;
			else
				return false;
		}
		else if (direction == 4) {
			if (client_smile.Y + 1 < rows - 1 && map_array[client_smile.Y + 1][client_smile.X] == '*')
				return true;
			else
				return false;
		}
		return false;
	}

	DWORD WINAPI Sender(void* param) {


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


			if (CanSmileMove(map_array, rows, cols, direction)) {
				if (IsCoinCollected(map_array, rows, cols, direction)) {
					client_coins++;
				}
				SetConsoleCursorPosition(h, client_smile);
				cout << ' ';

				if (code == 75) {//left
					client_smile.X--;
				}
				else if (code == 77) {//right
					client_smile.X++;
				}
				else if (code == 72) {//up
					client_smile.Y--;
				}
				else if (code == 80) {//down
					client_smile.Y++;
				}



				SetConsoleCursorPosition(h, client_smile);
				SetConsoleTextAttribute(h, 9);
				cout << '#';

				if (server_smile.X == client_smile.X && server_smile.Y == client_smile.Y) {
					system("title INTERSECTED!");
				}

				char message[200];
				strcpy_s(message, to_string(direction).c_str());
				send(ConnectSocket, message, strlen(message), 0);
			}




		}


		return 0;
	}

	void ShowMap(char** map_array, unsigned int height, unsigned int width) {
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				if (map_array[i][j] == ' ') {
					SetConsoleTextAttribute(h, 0);
					cout << ' ';
				}
				else if (map_array[i][j] == '|') {
					SetConsoleTextAttribute(h, 4);
					cout << '|';
				}
				else if (map_array[i][j] == '^') {
					server_smile.X = j;
					server_smile.Y = i;
					SetConsoleTextAttribute(h, 2);
					cout << '^'; //server
				}
				else if (map_array[i][j] == '#') {
					client_smile.X = j;
					client_smile.Y = i;
					SetConsoleTextAttribute(h, 9);
					cout << '#'; //client
				}
				else if (map_array[i][j] == '*')
				{
					SetConsoleTextAttribute(h, 14);
					cout << '*';
				}
			}
			cout << endl;
		}

	}





	void ParseData(char message[], char**& map_array, unsigned int& rows, unsigned int& cols) {
		string strMessage = message;
		int h_ind = strMessage.find('h');
		int w_ind = strMessage.find('w');
		int d_ind = strMessage.find('d');
		string strRows = "";
		if (w_ind == 2)
			strRows = strMessage.substr(1, 1);
		else
			strRows = strMessage.substr(1, 2);
		int rows_count = stoi(strRows);
		int distance = d_ind - w_ind - 1;
		string columns = strMessage.substr(w_ind + 1, distance);
		int columns_count = stoi(columns);

		rows = rows_count;
		cols = columns_count;
		string map_data = strMessage.substr(d_ind + 1, rows_count * columns_count);

		int map_current = 0;
		map_array = new char* [rows_count];
		for (int i = 0; i < rows_count; i++) {
			map_array[i] = new char[columns_count];
		}
		for (int i = 0; i < rows_count; i++) {
			for (int j = 0; j < columns_count; j++)
				map_array[i][j] = map_data[map_current++];
		}
	}

	DWORD WINAPI Receiver(LPVOID param) {
		char message[DEFAULT_BUFLEN];
		int iResult = recv(ConnectSocket, message, DEFAULT_BUFLEN, 0);
		message[iResult] = '\0';

		ParseData(message, map_array, rows, cols);
		ShowMap(map_array, rows, cols);


		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		while (!g_stopThreads) {
			if (g_stopThreads)
				TerminateThread(h, 0);
			char message[200];
			int iResult = recv(ConnectSocket, message, strlen(message), 0);
			message[iResult] = '\0';
			string strMessage = message;

			SetConsoleCursorPosition(h, server_smile);
			SetConsoleTextAttribute(h, 0);
			cout << ' ';
			if (iResult > 0) {
				if (strMessage == "1")
					server_smile.X--;
				else if (strMessage == "2")
					server_smile.X++;
				else if (strMessage == "3")
					server_smile.Y--;
				else if (strMessage == "4")
					server_smile.Y++;

				SetConsoleCursorPosition(h, server_smile);
				SetConsoleTextAttribute(h, 2);
				cout << '^';

				if (server_smile.X == client_smile.X && server_smile.Y == client_smile.Y) {
					system("title INTERSECTED!");
				}
			}


		}
		return 0;
	}