#include <iostream>
#include <map>
#include <unordered_map>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <mutex>
#include <array>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#include "protocol.h"
using namespace std;

enum OP_TYPE { OP_RECV, OP_SEND, OP_ACCEPT };
enum PL_STATE { PLST_FREE, PLST_CONNECTED, PLST_INGAME };

struct EX_OVER {
	WSAOVERLAPPED	m_over;
	WSABUF			m_wsaBuf[1];
	unsigned char	m_iobuf[MAX_BUFFER]; //netbuf, packetbuf ...
	OP_TYPE			m_op;

	SOCKET			m_csocket; //for accept
};

struct SESSION {
	//server data
	mutex m_s_lock;
	SOCKET m_socket;
	int m_id;
	EX_OVER m_recv_over;
	int m_prev_size;
	PL_STATE m_state;

	//game data
	char m_name[MAX_NAME_LEN];
	short x, y;
};

/// <summary>
/// SERVER_LISTEN_KEY + MAXUSER = MAXUSER+1
/// </summary>
array <SESSION, MAX_USER + 1> players;

constexpr int MAX_THREAD = 4;

void display_error(const char* msg, int err_no) {
	WCHAR* lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	cout << msg;
	wcout << lpMsgBuf << endl;
	LocalFree(lpMsgBuf);
}

void send_packet(int p_id, void* p) {
	int p_size = reinterpret_cast<unsigned char*>(p)[0]; //0???? size 
	EX_OVER* s_over = new EX_OVER;
	s_over->m_op = OP_SEND;
	memset(&s_over->m_over, 0, sizeof(s_over->m_over));
	memcpy(s_over->m_iobuf, p, p_size);
	s_over->m_wsaBuf[0].buf = reinterpret_cast<CHAR*>(s_over->m_iobuf);
	s_over->m_wsaBuf[0].len = p_size;
	int retval = WSASend(players[p_id].m_socket, s_over->m_wsaBuf, 1, NULL, 0, &s_over->m_over, 0);
	if (retval != 0) {
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING) display_error("WSASend: ", WSAGetLastError());
	}
}

void do_recv(int Session_id) {
	SESSION& s = players[Session_id];
	s.m_recv_over.m_wsaBuf[0].buf = reinterpret_cast<char*>(s.m_recv_over.m_iobuf) + s.m_prev_size;
	s.m_recv_over.m_wsaBuf[0].len = MAX_BUFFER - s.m_prev_size;
	ZeroMemory(&s.m_recv_over.m_over, sizeof(s.m_recv_over.m_over));
	DWORD recv_flag = 0;
	int retval = WSARecv(s.m_socket, &s.m_recv_over.m_wsaBuf[0], 1, NULL, &recv_flag, &s.m_recv_over.m_over, NULL);

	if (retval != 0) {
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING) display_error("WSASend: ", WSAGetLastError());
	}
}

void send_move_packet(int to, int from) {
	s2c_packet_move_player p;
	p.id = from;
	p.size = sizeof(p);
	p.type = SC_MOVE_PLAYER;
	p.x = players[from].x;
	p.y = players[from].y;

	send_packet(to, &p);
}

void do_move(int p_id, int dir) {
	SESSION& p = players[p_id];
	switch (dir) {
	case D_R: if (p.x < (WORLD_X_SIZE - 1)) ++p.x;		break;
	case D_D: if (p.y < (WORLD_Y_SIZE - 1)) ++p.y;		break;
	case D_L: if (p.x > 0) --p.x;						break;
	case D_U: if (p.y > 0) --p.y;						break;
	}
	for (auto& pl : players) {
		pl.m_s_lock.lock();
		if (pl.m_state == PLST_INGAME) {
			send_move_packet(pl.m_id, p_id);
		}
		pl.m_s_lock.unlock();
	}
}

int get_new_player_id(SOCKET p_socket) {
	for (int i = SERVER_LISTEN_KEY + 1; i <= MAX_USER; ++i) {
		players[i].m_s_lock.lock();
		if (players[i].m_state == PLST_FREE) {
			players[i].m_state = PLST_CONNECTED;
			players[i].m_socket = p_socket;
			players[i].m_name[0] = 0;
			players[i].m_s_lock.unlock();
			return i;
		}
		players[i].m_s_lock.unlock();
	}
	return -1;
}

void send_login_okay_packet(int p_id) {
	s2c_packet_login_okay p;
	p.size = sizeof(p);
	p.type = SC_LOGIN_OKAY;

	p.hp = 10;
	p.id = p_id;
	p.level = 2;
	p.race = 1;
	p.x = players[p_id].x;
	p.y = players[p_id].y;

	send_packet(p_id, &p);
}

void send_add_player(int to, int from) {
	s2c_packet_add_player p;
	p.id = from;
	p.size = sizeof(p);
	p.type = SC_ADD_PLAYER;
	p.x = players[from].x;
	p.y = players[from].y;
	p.race = 0;
	send_packet(to, &p);
}

void send_remove_player(int to, int from) {
	s2c_packet_remove_player p;
	p.id = from;
	p.size = sizeof(p);
	p.type = SC_REMOVE_PLAYER;
	send_packet(to, &p);
}

void process_packet(int p_id, unsigned char* p_buf) {
	switch (p_buf[1]) { //type
	case CS_LOGIN: {
		c2s_packet_login* p = reinterpret_cast<c2s_packet_login*>(p_buf);
		players[p_id].m_s_lock.lock();
		strcpy_s(players[p_id].m_name, p->name);
		send_login_okay_packet(p_id);

		players[p_id].m_state = PLST_INGAME;
		for (auto& p : players) {
			if (p.m_state == PLST_INGAME && p.m_id != p_id) {
				p.m_s_lock.lock();
				if (p.m_state == PLST_INGAME) {
					send_add_player(p_id, p.m_id);
					send_add_player(p.m_id, p_id);
				}
				p.m_s_lock.unlock();
			}
		}
		players[p_id].m_s_lock.unlock();

		break;
	}
	case CS_MOVE: {
		c2s_packet_move* p = reinterpret_cast<c2s_packet_move*>(p_buf);
		do_move(p_id, p->dir);
		break;
	}
	default: {
		cout << "Unknown Packet Type from Client [" << p_id << "] Packet Type [" << p_buf[1] << "]\n";
		while (true); //????
		break;
	}
	}
}

void disconnect(int p_id) {
	players[p_id].m_s_lock.lock();
	closesocket(players[p_id].m_socket);
	players[p_id].m_state = PLST_FREE;
	players[p_id].m_s_lock.unlock();
	for (auto& pl : players) {
		if (pl.m_id == p_id) continue;
		pl.m_s_lock.lock();
		if (pl.m_state == PLST_INGAME) {
			send_remove_player(pl.m_id, p_id);
		}
		pl.m_s_lock.unlock();
	}
}

void worker(HANDLE h_iocp, SOCKET listen_socket) {
	while (true) {
		DWORD io_bytes;
		ULONG_PTR iocp_key;
		WSAOVERLAPPED* over;

		BOOL ret = GetQueuedCompletionStatus(h_iocp, &io_bytes, &iocp_key, &over, INFINITE);
		int key = static_cast<int>(iocp_key);

		if (ret == FALSE) {
			if (SERVER_LISTEN_KEY == key) {
				display_error("GQCS: ", WSAGetLastError());
				exit(-1);
			}
			else { //????????
				display_error("GQCS: ", WSAGetLastError());
				disconnect(key);
			}
		}
		if ((key != SERVER_LISTEN_KEY)&& (io_bytes == 0)) { //????????
			disconnect(key);
			continue;
		}

		EX_OVER* ex_over = reinterpret_cast<EX_OVER*>(over);

		switch (ex_over->m_op) {
		case OP_RECV: {
			unsigned char* packet_ptr = reinterpret_cast<unsigned char*>(ex_over->m_iobuf); //???? ??????
			int num_data = io_bytes + players[key].m_prev_size; //???? ?????? = ?????°? + ?????? ?ִ? ũ??
			int packet_size = packet_ptr[0];		//ó???? ?????Դٸ?, packet_size?? packet_ptr[0](size)
			while (num_data >= packet_size) {		//???? ?????? >= ??Ŷũ??
				process_packet(key, packet_ptr);	//??Ŷ ????
				num_data -= packet_size;			//???굥???? - ??Ŷũ?? = ???굥????
				packet_ptr += packet_size;			//???? ?????? + ??Ŷ?????ŭ ?ڷ? ?̵?
				packet_size = packet_ptr[0];		//??Ŷ???????? packet_ptr[0]
				if (num_data <= 0) break;
			}
			players[key].m_prev_size = num_data;	//?????????? = ???굥????
			if (num_data != 0) memcpy(ex_over->m_iobuf, packet_ptr, num_data);		//io_buf?? packet_ptr?? num_data??ŭ ????
			do_recv(key);
			break;
		}
		case OP_SEND: {
			//?? ?????? ?˻?
			delete ex_over;
			break;
		}
		case OP_ACCEPT: {
			int client_id = get_new_player_id(ex_over->m_csocket);
			if (client_id != -1) {
				SESSION& p = players[client_id];
				p.m_recv_over.m_op = OP_RECV;
				p.m_prev_size = 0;
				p.x = 0;
				p.y = 0;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(p.m_socket), h_iocp, client_id, 0);

				do_recv(client_id);
			}
			else {
				closesocket(players[client_id].m_socket);
			}

			//accpet ?????Ǹ? ???ο? accept ????
			ZeroMemory(&ex_over->m_over, sizeof(ex_over->m_over));
			SOCKET client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			ex_over->m_csocket = client_socket;
			int ret = AcceptEx(listen_socket, client_socket, ex_over->m_iobuf, 0, 32, 32, NULL, &ex_over->m_over);

			if (ret != TRUE) {
				int err_num = WSAGetLastError();
				if (err_num != WSA_IO_PENDING) {
					display_error("Accept: ", err_num);
				}
			}

			break;
		}
		}
	}
}

int main()
{
	for (int i = 0; i < MAX_USER; ++i) {
		players[i].m_id = i;
		players[i].m_state = PLST_FREE;
	}
	wcout.imbue(locale("korean"));
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, NULL);

	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(listenSocket), h_iocp, 0, NULL);//0???? listen socket

	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
	listen(listenSocket, SOMAXCONN);

	EX_OVER accept_over;
	accept_over.m_op = OP_ACCEPT;
	ZeroMemory(&accept_over.m_over, sizeof(accept_over.m_over));
	SOCKET client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	accept_over.m_csocket = client_socket;
	int ret = AcceptEx(listenSocket, client_socket, accept_over.m_iobuf, 0, 32, 32, NULL, &accept_over.m_over);

	if (ret != TRUE) {
		int err_num = WSAGetLastError();
		if (err_num != WSA_IO_PENDING) {
			display_error("Accept: ", err_num);
		}
	}

	vector<thread> vt_worker;
	for (int i = 0; i < MAX_THREAD; ++i) {
		vt_worker.emplace_back(worker, h_iocp, listenSocket);
	}
	for (auto& th : vt_worker) {
		th.join();
	}

	closesocket(listenSocket);
	WSACleanup();
}