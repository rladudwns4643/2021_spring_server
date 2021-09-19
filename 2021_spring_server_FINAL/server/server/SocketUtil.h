#pragma once
#include "TCPSocket.h"

enum SocketAddressFamily { //enum µÐ°©¼ú
	INET = AF_INET,
	INET6 = AF_INET6
};

class SocketUtil {
public:
	static bool StaticInit() {
		WSADATA wsa;
		int retval = WSAStartup(MAKEWORD(2, 2), &wsa);
		if (retval != NO_ERROR) {
			assert(!"SOCKUTIL - INIT");
			return false;
		}
		return true;
	}

	static void CleanUp() {
		WSACleanup();
	}

	static TCPSocketPtr CreateTCPSocket(SocketAddressFamily f) {
		SOCKET s = WSASocket(f, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (s != INVALID_SOCKET) {
#ifdef LOG
			std::cout << "Create TCP Socket\n";
#endif
			return TCPSocketPtr(new TCPSocket(s));
		}
		else {
			assert(!"SocketUtil::CreateTCPSocket");
			return nullptr;
		}
	}
};