#include <iostream>
#include <map>
using namespace std;
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define MAX_BUFFER        1024
#define SERVER_PORT        3500

struct SOCKETINFO
{
	WSAOVERLAPPED overlapped;
	WSABUF dataBuffer;
	SOCKET socket;
	char messageBuffer[MAX_BUFFER];
};

map <SOCKET, SOCKETINFO> clients;

//CompletionROUTINE recv 사용자 정의 함수
void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
//CompletionROUTINE send 사용자 정의 함수
void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);

void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	SOCKET client_s = reinterpret_cast<SOCKETINFO*>(overlapped)->socket;

	if (dataBytes == 0)
	{
		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		return;
	}  // 클라이언트가 closesocket을 했을 경우
	clients[client_s].messageBuffer[dataBytes] = 0;
	cout << "From client : " << clients[client_s].messageBuffer << " (" << dataBytes << ") bytes)\n";
	clients[client_s].dataBuffer.len = dataBytes;
	memset(&(clients[client_s].overlapped), 0, sizeof(WSAOVERLAPPED));
	WSASend(client_s, &(clients[client_s].dataBuffer), 1, NULL, 0, overlapped, send_callback);
}

void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	SOCKET client_s = reinterpret_cast<SOCKETINFO *>(overlapped)->socket;

	if (dataBytes == 0) {
		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		return;
	}  // 클라이언트가 closesocket을 했을 경우

	cout << "TRACE - Send message : " << clients[client_s].messageBuffer << " (" << dataBytes << " bytes)\n";
	memset(&(clients[client_s].overlapped), 0, sizeof(WSAOVERLAPPED));
	clients[client_s].dataBuffer.len = MAX_BUFFER;

	DWORD flags = 0;
	WSARecv(client_s, &clients[client_s].dataBuffer, 1, 0, &flags, overlapped, recv_callback);
}

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData); //overlapped IO를 위해서 2.2버전을 사용해야 한다
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
	listen(listenSocket, 5);
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	memset(&clientAddr, 0, addrLen);

	while (true) {
		SOCKET clientSocket = accept(listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
		clients[clientSocket] = SOCKETINFO{};
		clients[clientSocket].socket = clientSocket;
		clients[clientSocket].dataBuffer.len = MAX_BUFFER;
		clients[clientSocket].dataBuffer.buf = clients[clientSocket].messageBuffer;
		memset(&clients[clientSocket].overlapped, 0, sizeof(WSAOVERLAPPED));
		DWORD flags = 0;
		WSARecv(clients[clientSocket].socket, &clients[clientSocket].dataBuffer, 1, NULL,
			&flags, &(clients[clientSocket].overlapped), recv_callback);
	}
	closesocket(listenSocket);
	WSACleanup();
}

