#include <iostream>
#include <WS2tcpip.h>
#include <unordered_map>
using namespace std;
#pragma comment(lib, "WS2_32.lib")

constexpr int BUF_SIZE = 1024;
constexpr int PORT = 3500;
constexpr int MAX_BUFFER = 1024;
constexpr int MAX_USER = 10;
constexpr int init_X = 5;
constexpr int init_Y = 5;

enum class e_KEY { LEFT = 1, RIGHT = 2, UP = 3, DOWN = 4, K_NULL = 5 };

struct User {
	int id;

	int x;
	int y;
	bool connect;
};

struct KEY {
	int id;
	e_KEY k;
};

struct EXOVER {
	WSAOVERLAPPED over;
	WSABUF dataBuffer;
	SOCKET socket;
	char msgBuffer[sizeof(User)];
};

unordered_map <SOCKET, EXOVER> clients;

User userlist[MAX_USER];

void err_display(const char* msg, int err_no) {
	WCHAR* m_msg;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&m_msg, 0, NULL);
	cout << msg;
	wcout << L"  에러: " << m_msg << endl;
	while (true);
	LocalFree(m_msg);
}
void KeyMsg(e_KEY* key, User& u) {
	switch (*key) {
	case e_KEY::LEFT: {
		u.x -= 50;
		if (u.x < 0) u.x = 0;
		break;
	}
	case e_KEY::RIGHT: {
		u.x += 50;
		if (u.x > 400) u.x = 400;
		break;
	}
	case e_KEY::UP: {
		u.y -= 50;
		if (u.y > 400) u.y = 400;
		break;
	}
	case e_KEY::DOWN: {
		u.y += 50;
		if (u.y < 0) u.y = 0;
		break;
	}
	case e_KEY::K_NULL: {

		break;
	}
	default: break;
	}
}
void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags) {
	SOCKET c = reinterpret_cast<EXOVER*>(overlapped)->socket;

	KEY k;
	memcpy(&k, clients[c].msgBuffer, sizeof(KEY));
	if (dataBytes == 0) {
		closesocket(clients[c].socket);
		clients.erase(c);
		userlist[k.id].connect = false;
		return;
	}

	clients[c].msgBuffer[dataBytes] = 0;
	KeyMsg(&k.k, userlist[k.id]);
	clients[c].dataBuffer.len = sizeof(userlist);
	clients[c].dataBuffer.buf = (char*)&userlist;
	
	ZeroMemory(&clients[c].over, sizeof(WSAOVERLAPPED));
	WSASend(c, &clients[c].dataBuffer, 1, &dataBytes, 0, &(clients[c].over), send_callback);
}
void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags) {
	SOCKET c = reinterpret_cast<EXOVER*>(overlapped)->socket;

	if (dataBytes == 0) {
		closesocket(clients[c].socket);
		clients.erase(c);
		return;
	}
	clients[c].dataBuffer.len = MAX_BUFFER;
	clients[c].dataBuffer.buf = clients[c].msgBuffer;
	ZeroMemory(&(clients[c].over), sizeof(WSAOVERLAPPED));
	DWORD flags = 0;
	WSARecv(c, &clients[c].dataBuffer, 1, 0, &flags, &clients[c].over, recv_callback);	
}
int main() {
	wcout.imbue(std::locale("korean"));
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN s_a;
	ZeroMemory(&s_a, sizeof(s_a));
	s_a.sin_family = AF_INET;
	s_a.sin_port = htons(PORT);
	s_a.sin_addr.S_un.S_addr = INADDR_ANY;
	::bind(listenSocket, (sockaddr*)&s_a, sizeof(s_a));
	listen(listenSocket, MAX_USER/*SOMAXCONN*/);

	SOCKADDR_IN c_a;
	int addrSize = sizeof(c_a);
	ZeroMemory(&c_a, sizeof(SOCKADDR_IN));

	while (true) {
		if (clients.size() > MAX_USER) continue;
		SOCKET c = accept(listenSocket, reinterpret_cast<sockaddr*>(&c_a), &addrSize);
		if (SOCKET_ERROR == c) { err_display("WSAAccprt", WSAGetLastError()); };
		int cur = -1;
		for (int i = 0; i < MAX_USER; ++i) {
			if (userlist[i].connect == false) {
				cur = i;
				cout << "new player " << i << endl;
				break;
			}
		}

		userlist[cur] = User{};
		
		userlist[cur].connect = true;
		userlist[cur].x = init_X* 50 ;
		userlist[cur].y = init_Y * 50;
		userlist[cur].id = cur;

		clients[c] = EXOVER{};
		clients[c].socket = c;
		clients[c].dataBuffer.len = sizeof(User);
		clients[c].dataBuffer.buf = (char*)&userlist[cur];
		ZeroMemory(&clients[c].over, sizeof(WSAOVERLAPPED));
		DWORD flags = 0;
		WSASend(clients[c].socket, &clients[c].dataBuffer, 1, NULL, 0, &(clients[c].over), send_callback); //초기값 먼저 보내줘야 함
	}
	closesocket(listenSocket);
	WSACleanup();
}