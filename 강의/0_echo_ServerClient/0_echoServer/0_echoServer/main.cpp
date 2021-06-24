//210312 class example

#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")

using namespace std;

constexpr int BUF_SIZE = 256;
constexpr int PORTNUM = 3500;
const char* IPNUM = "127.0.0.1";

int main() {
	WSAData wsa;
	WSAStartup(MAKEWORD(2, 0), &wsa);
	SOCKET s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
	SOCKADDR_IN s_a;
	ZeroMemory(&s_a, sizeof(SOCKADDR_IN));
	s_a.sin_family = AF_INET;
	s_a.sin_port = htons(PORTNUM);
	s_a.sin_addr.S_un.S_addr = INADDR_ANY;
	int s_addrlen = sizeof(s_a);
	::bind(s, reinterpret_cast<sockaddr*>(&s_a), s_addrlen);
	listen(s, SOMAXCONN);

	SOCKADDR_IN c_a;
	ZeroMemory(&c_a, sizeof(c_a));
	int c_addrlen = sizeof(c_a);
	SOCKET c = WSAAccept(s, reinterpret_cast<sockaddr*>(&c_a), &c_addrlen, NULL, NULL);

	while (1) {
		char r_msg[BUF_SIZE];
		WSABUF r_wsabuf[1];
		r_wsabuf[0].buf = r_msg;
		r_wsabuf[0].len = BUF_SIZE;
		DWORD bytes_recv;
		DWORD recv_flag = 0;
		WSARecv(c, r_wsabuf, 1, &bytes_recv, &recv_flag, NULL, NULL);
		cout << "Client msg: " << r_msg << endl;

		DWORD bytes_sent;
		WSASend(c, r_wsabuf, 1, &bytes_sent, 0, NULL, NULL);
	}
	closesocket(s);
	WSACleanup();
}