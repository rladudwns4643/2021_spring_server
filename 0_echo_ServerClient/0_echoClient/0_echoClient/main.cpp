//210312 class example

#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")

using namespace std;

constexpr int BUF_SIZE = 256;
constexpr int PORTNUM = 3500;
const char* IPNUM = "127.0.0.1";

void display_error(const char* msg, int err_no) {
	WCHAR* lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
		NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		(LPTSTR)&lpMsgBuf, 0, NULL);
	cout << msg;
	wcout << lpMsgBuf << endl;
	LocalFree(lpMsgBuf);
}

int main() {

	wcout.imbue(locale("korean"));
	WSAData wsa;
	WSAStartup(MAKEWORD(2, 0), &wsa);
	SOCKET s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
	SOCKADDR_IN s_a;
	ZeroMemory(&s_a, sizeof(s_a));
	s_a.sin_family = AF_INET;
	s_a.sin_port = htons(PORTNUM);
	inet_pton(AF_INET, IPNUM, &s_a.sin_addr); //ip -> num
	WSAConnect(s, reinterpret_cast<sockaddr*>(&s_a) , sizeof(s_a), 0, 0, 0, 0);
	while (1) {
		char msg[BUF_SIZE];
		cout << "Enter Msg: ";
		cin.getline(msg, BUF_SIZE - 1/* -1 for last \0*/);

		WSABUF s_wsabuf[1]; //동시에 하나의 버퍼를 전송할 수 있다
		s_wsabuf[0].buf = msg;
		s_wsabuf[0].len = static_cast<unsigned int>(strlen(msg) + 1)/* +1 for last \0*/;
		DWORD bytes_sent;
		WSASend(s, s_wsabuf, 1, &bytes_sent, 0, NULL, NULL);

		char r_msg[BUF_SIZE];
		WSABUF r_wsabuf[1];
		r_wsabuf[0].buf = r_msg;
		r_wsabuf[0].len = BUF_SIZE;
		DWORD bytes_recv;
		DWORD recv_flag = 0;
		int ret = WSARecv(s, r_wsabuf, 1, &bytes_recv, &recv_flag, NULL, NULL);
		if (SOCKET_ERROR == ret) {
			display_error("recv error: ", WSAGetLastError());
			exit(-1);
		}
		cout << "Server msg: " << r_msg << endl;
	}
	closesocket(s);
	WSACleanup();
}