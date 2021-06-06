#include <iostream>
#include <unordered_set>
#include <thread>
#include <vector>
#include <mutex>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <queue>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "MSWSock.lib")

#include "protocol.h"
using namespace std;
using namespace chrono;

//#define LOG_ON

enum OP_TYPE  { OP_RECV, OP_SEND, OP_ACCEPT, OP_RANDOM_MOVE, OP_ATTACK };
enum PL_STATE { PLST_FREE, PLST_CONNECTED, PLST_INGAME };
struct EX_OVER
{
	WSAOVERLAPPED	m_over;
	WSABUF			m_wsabuf[1];
	unsigned char	m_packetbuf[MAX_BUFFER];
	OP_TYPE			m_op;
	SOCKET			m_csocket;					// OP_ACCEPT에서만 사용
};
struct S_OBJECT
{
	mutex  m_slock;
	atomic<PL_STATE> m_state;
	SOCKET m_socket;
	int		id;

	EX_OVER m_recv_over;
	int m_prev_size;

	char m_name[200];
	short	x, y;
	unsigned int move_time;
	atomic_bool is_active;

	unordered_set<int> m_view_list;
	mutex m_view_lock;

};
struct TIMER_EVENT {
	int object;				//누가
	OP_TYPE e_type;			//무엇을
	system_clock::time_point start_time;		//언제
	int target_id;

	constexpr bool operator < (const TIMER_EVENT& lhs) const {
		return start_time > lhs.start_time;
	}
};
priority_queue<TIMER_EVENT> timer_queue;
mutex timer_lock;

constexpr int SERVER_ID = 0;
array <S_OBJECT, MAX_USER + 1> objects;
HANDLE h_iocp;

bool CAS(volatile atomic_bool* addr, int before, int after) {
	return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(addr), &before, after);
}

void add_event(int obj, OP_TYPE ev_t, int delay_ms) {
	TIMER_EVENT ev;
	ev.e_type = ev_t;
	ev.object = obj;
	ev.start_time = system_clock::now() + milliseconds(delay_ms);
	timer_lock.lock();
	timer_queue.push(ev);
	timer_lock.unlock();
}

void wake_up_npc(int npc_id) {
	if (objects[npc_id].is_active == false) {
		if (CAS(&objects[npc_id].is_active, false, true) == true) {
			add_event(npc_id, OP_RANDOM_MOVE, 1000);
		}
	}
}

void disconnect(int p_id);

void display_error(const char* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	cout << msg;
	wcout << lpMsgBuf << endl;
	LocalFree(lpMsgBuf);
}

void send_packet(int p_id, void *p)
{
	int p_size = reinterpret_cast<unsigned char*>(p)[0];
	int p_type = reinterpret_cast<unsigned char*>(p)[1];
#ifdef LOG_ON
	cout << "To client [" << p_id << "] : ";
	cout << "Packet [" << p_type << "]\n";
#endif
	EX_OVER* s_over = new EX_OVER;
	s_over->m_op = OP_SEND;
	memset(&s_over->m_over, 0, sizeof(s_over->m_over));
	memcpy(s_over->m_packetbuf, p, p_size);
	s_over->m_wsabuf[0].buf = reinterpret_cast<CHAR *>(s_over->m_packetbuf);
	s_over->m_wsabuf[0].len = p_size;
	int ret = WSASend(objects[p_id].m_socket, s_over->m_wsabuf, 1, 
		NULL, 0, &s_over->m_over, 0);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no) {
			display_error("WSASend : ", WSAGetLastError());
			disconnect(p_id);
		}
	}
}

bool is_npc(int id) {
	if (id >= NPC_ATTRIB) return true;
	return false;
}

void do_recv(int key)
{
	objects[key].m_recv_over.m_wsabuf[0].buf =
		reinterpret_cast<char *>(objects[key].m_recv_over.m_packetbuf)
		+ objects[key].m_prev_size;
	objects[key].m_recv_over.m_wsabuf[0].len = MAX_BUFFER - objects[key].m_prev_size;
	memset(&objects[key].m_recv_over.m_over, 0, sizeof(objects[key].m_recv_over.m_over));
	DWORD r_flag = 0;
	int ret = WSARecv(objects[key].m_socket, objects[key].m_recv_over.m_wsabuf, 1,
		NULL, &r_flag, &objects[key].m_recv_over.m_over, NULL);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			display_error("WSARecv : ", WSAGetLastError());
	}
}

int get_new_player_id(SOCKET p_socket)
{
	int idx;

	for (idx = 0; idx < NPC_ATTRIB; ++idx) {
		if (objects[idx].m_state == PLST_FREE) break;
	}
	if (idx == NPC_ATTRIB) {
		closesocket(p_socket); //limit new player
	}
	else {
		objects[idx].m_slock.lock();
		objects[idx].m_state = PLST_CONNECTED;
		objects[idx].m_socket = p_socket;
		objects[idx].m_name[0] = 0;
		objects[idx].m_slock.unlock();
		return idx;
	}
	return -1;
}

void send_login_ok_packet(int p_id)
{
	s2c_login_ok p;
	p.hp = 10;
	p.id = p_id;
	p.level = 2;
	p.race = 1;
	p.size = sizeof(p);
	p.type = S2C_LOGIN_OK;
	p.x = objects[p_id].x;
	p.y = objects[p_id].y;
	send_packet(p_id, &p);
}

void send_move_packet(int c_id, int p_id)
{
	s2c_move_player p;
	p.id = p_id;
	p.size = sizeof(p);
	p.type = S2C_MOVE_PLAYER;
	p.x = objects[p_id].x;
	p.y = objects[p_id].y;
	p.move_time = objects[p_id].move_time;
	send_packet(c_id, &p);
}

void send_add_object(int c_id, int p_id)
{
	s2c_add_player p;
	p.id = p_id;
	p.size = sizeof(p);
	p.type = S2C_ADD_PLAYER;
	p.x = objects[p_id].x;
	p.y = objects[p_id].y;
	p.race = 0;
	strcpy_s(p.name, objects[p_id].m_name);
	send_packet(c_id, &p);
}

void send_remove_object(int c_id, int p_id)
{
	s2c_remove_player p;
	p.id = p_id;
	p.size = sizeof(p);
	p.type = S2C_REMOVE_PLAYER;
	send_packet(c_id, &p);
}

bool can_see(int id_r, int id_l) {
	return VIEW_RADIUS >= abs(objects[id_r].x - objects[id_l].x)
		&& VIEW_RADIUS >= abs(objects[id_r].y - objects[id_l].y);
}

void do_move(int p_id, DIRECTION dir)
{
	auto& x = objects[p_id].x;
	auto& y = objects[p_id].y;
	switch (dir) {
	case D_N: if (y > 0) y--; break;
	case D_S: if (y < (WORLD_Y_SIZE - 1)) y++; break;
	case D_W: if (x > 0) x--; break;
	case D_E: if (x < (WORLD_X_SIZE - 1)) x++; break;
	}
	unordered_set<int> old_vl;
	objects[p_id].m_view_lock.lock();
	old_vl = objects[p_id].m_view_list;
	objects[p_id].m_view_lock.unlock();
	unordered_set<int> new_vl;
	for (auto& pl : objects) {
		if (pl.id == p_id) continue;
		if ((pl.m_state == PLST_INGAME) && can_see(p_id, pl.id))
			new_vl.insert(pl.id);
	}
	send_move_packet(p_id, p_id);
	for (auto pl : new_vl) {
		if (0 == old_vl.count(pl)) {      // 1. 새로 시야에 들어오는 경우
			objects[p_id].m_view_lock.lock();
			objects[p_id].m_view_list.insert(pl);
			objects[p_id].m_view_lock.unlock();
			send_add_object(p_id, pl);

			if (false == is_npc(pl)) {
				objects[pl].m_view_lock.lock();
				if (0 == objects[pl].m_view_list.count(p_id)) {
					objects[pl].m_view_list.insert(p_id);
					objects[pl].m_view_lock.unlock();
					send_add_object(pl, p_id);
				}
				else {
					objects[pl].m_view_lock.unlock();
					send_move_packet(pl, p_id);
				}
			}
			else { // is_npc(pl) == true
				wake_up_npc(pl);
			}
		}
		else {                        // 2. 기존 시야에도 있고 새 시야에도 있는 경우
			if (false == is_npc(pl)) {
				objects[pl].m_view_lock.lock();
				if (0 == objects[pl].m_view_list.count(p_id)) {
					objects[pl].m_view_list.insert(p_id);
					objects[pl].m_view_lock.unlock();
					send_add_object(pl, p_id);
				}
				else {
					objects[pl].m_view_lock.unlock();
					send_move_packet(pl, p_id);
				}
				//send_move_packet(pl, p_id);
			}
		}
	}

	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			// 3. 시야에서 사라진 경우
			objects[p_id].m_view_lock.lock();
			objects[p_id].m_view_list.erase(pl);
			objects[p_id].m_view_lock.unlock();
			send_remove_object(p_id, pl);

			if (false == is_npc(pl)) {
				objects[pl].m_view_lock.lock();
				if (0 != objects[pl].m_view_list.count(p_id)) {
					objects[pl].m_view_list.erase(p_id);
					objects[pl].m_view_lock.unlock();
					send_remove_object(pl, p_id);
				}
				else {
					objects[pl].is_active = false;
					objects[pl].m_view_lock.unlock();
				}
			}
		}
	}
}

void process_packet(int p_id, unsigned char* p_buf)
{
	switch (p_buf[1]) {
	case C2S_LOGIN: {
		c2s_login* packet = reinterpret_cast<c2s_login*>(p_buf);
		objects[p_id].m_slock.lock();
		strcpy_s(objects[p_id].m_name, packet->name);
		objects[p_id].x = rand() % WORLD_X_SIZE;
		objects[p_id].y = rand() % WORLD_Y_SIZE;
		objects[p_id].m_state = PLST_INGAME;
		objects[p_id].m_slock.unlock();
		send_login_ok_packet(p_id);

		for (auto& pl : objects) {
			if (p_id != pl.id) {
				//lock_guard <mutex> gl{ pl.m_slock };
				if (PLST_INGAME == pl.m_state) {
					if (can_see(p_id, pl.id)) {
						objects[p_id].m_view_lock.lock();
						objects[p_id].m_view_list.insert(pl.id);
						objects[p_id].m_view_lock.unlock();
						send_add_object(p_id, pl.id);
						if (is_npc(pl.id) == false) {
							objects[pl.id].m_view_lock.lock();
							objects[pl.id].m_view_list.insert(p_id);
							send_add_object(pl.id, p_id);
							objects[pl.id].m_view_lock.unlock();
						}
						else {
							wake_up_npc(pl.id);
						}
					}
				}
			}
		}
		break;
	}
	case C2S_MOVE: {
		c2s_move* packet = reinterpret_cast<c2s_move*>(p_buf);
		objects[p_id].move_time = packet->move_time;

		do_move(p_id, packet->dir);
		break;
	}
	default:
#ifdef LOG_ON
		cout << "Unknown Packet Type from Client[" << p_id;
		cout << "] Packet Type [" << p_buf[1] << "]";
#endif
		while (true);
	}
}

void disconnect(int p_id)
{
	objects[p_id].m_slock.lock();
	if (objects[p_id].m_state == PLST_FREE) return;
	closesocket(objects[p_id].m_socket);
	objects[p_id].m_state = PLST_FREE;
	objects[p_id].m_slock.unlock();

	for (int i = 0; i < NPC_ATTRIB; ++i) {
		if (is_npc(objects[i].id) == true) continue;
		if (p_id == objects[i].id) continue;
		lock_guard<mutex> gl2{ objects[i].m_slock };
		if (PLST_INGAME == objects[i].m_state) {
			send_remove_object(objects[i].id, p_id);
		}
	}
}

void do_npc_random_move(S_OBJECT& npc) {
	if (npc.is_active == false) return;

	unordered_set<int> old_vl;
	for (int i = 0; i < NPC_ATTRIB; i++) { //all object -> all user
		if (PLST_INGAME != objects[i].m_state) continue;
		if (can_see(npc.id, objects[i].id) == false) continue;
		old_vl.insert(objects[i].id);
	}

	int x = npc.x;
	int y = npc.y;
	switch (rand() % 4) {
	case 0: {
		if (x < (WORLD_X_SIZE - 1))x++;
		break;
	}
	case 1: {
		if (x > 0)x--;
		break;
	}
	case 2: {
		if (y < (WORLD_Y_SIZE - 1))y++;
		break;
	}
	case 3: {
		if (y > 0)y--;
		break;
	}
	}
	npc.x = x;
	npc.y = y;

	unordered_set<int> new_vl;
	for (int i = 0; i < NPC_ATTRIB; i++) { //all object -> all user
		if (PLST_INGAME != objects[i].m_state) continue;
		if (can_see(npc.id, objects[i].id) == false) continue;
		new_vl.insert(objects[i].id);
	}

	//새로 추가된 객체
	for (auto pl : new_vl) {
		if (old_vl.count(pl) == 0) {
			//플레이어의 시야에 등장
			objects[pl].m_view_lock.lock();
			objects[pl].m_view_list.insert(npc.id);
			objects[pl].m_view_lock.unlock();
			send_add_object(pl, npc.id);
		}
		else {
			//플레이어의 시야에서 이동
			send_move_packet(pl, npc.id);
		}
	}

	for (auto pl : old_vl) {
		if (new_vl.count(pl) == 0) {
			objects[pl].m_view_lock.lock();
			if (objects[pl].m_view_list.count(pl) != 0) {
				objects[pl].m_view_list.erase(npc.id);
				objects[pl].m_view_lock.unlock();
				send_remove_object(pl, npc.id);
			}
			else {
				objects[pl].m_view_lock.unlock();
			}
		}
	}
}

void do_worker(HANDLE h_iocp, SOCKET l_socket)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR ikey;
		WSAOVERLAPPED* over;

		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes,
			&ikey, &over, INFINITE);

		int key = static_cast<int>(ikey);
		if (FALSE == ret) {
			if (SERVER_ID == key) {
				display_error("GQCS : ", WSAGetLastError());
				exit(-1);
			}
			else {
				display_error("GQCS : ", WSAGetLastError());
				disconnect(key);
			}
		}
		if ((key != SERVER_ID) && (0 == num_bytes)) {
			disconnect(key);
			continue;
		}
		EX_OVER* ex_over = reinterpret_cast<EX_OVER*>(over);

		switch (ex_over->m_op) {
		case OP_RECV: {
			unsigned char* packet_ptr = ex_over->m_packetbuf;
			int num_data = num_bytes + objects[key].m_prev_size;
			int packet_size = packet_ptr[0];

			while (num_data >= packet_size) {
				process_packet(key, packet_ptr);
				num_data -= packet_size;
				packet_ptr += packet_size;
				if (0 >= num_data) break;
				packet_size = packet_ptr[0];
			}
			objects[key].m_prev_size = num_data;
			if (0 != num_data)
				memcpy(ex_over->m_packetbuf, packet_ptr, num_data);
			do_recv(key);
			break;
		}
		case OP_SEND: {
			delete ex_over;
			break;
		}
		case OP_ACCEPT: {
			int c_id = get_new_player_id(ex_over->m_csocket);
			if (-1 != c_id) {
				objects[c_id].m_recv_over.m_op = OP_RECV;
				objects[c_id].m_prev_size = 0;
				CreateIoCompletionPort(
					reinterpret_cast<HANDLE>(objects[c_id].m_socket), h_iocp, c_id, 0);
				do_recv(c_id);
			}
			else closesocket(objects[c_id].m_socket);

			memset(&ex_over->m_over, 0, sizeof(ex_over->m_over));
			SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			ex_over->m_csocket = c_socket;
			AcceptEx(l_socket, c_socket, ex_over->m_packetbuf, 0, 32, 32, NULL, &ex_over->m_over);
			break;
		}
		case OP_RANDOM_MOVE: { //무조건 이동하는 것이 아닌 플레이어가 주변에 있을 때만 이동
			do_npc_random_move(objects[key]);
			add_event(key, OP_RANDOM_MOVE, 1000);
			delete ex_over;
			break;
		}
		case OP_ATTACK: break;
		}
	}
}

void do_timer() {
	using namespace chrono;

	while (1) {
		timer_lock.lock();
		if ((timer_queue.empty() == false) && timer_queue.top().start_time < system_clock::now()) {
			TIMER_EVENT ev{ timer_queue.top() };
			timer_queue.pop();
			timer_lock.unlock();
			EX_OVER* ex_over = new EX_OVER;
			ex_over->m_op = ev.e_type;
			ex_over->m_over;
			PostQueuedCompletionStatus(h_iocp, 1, ev.object, &ex_over->m_over);
		}
		else {
			timer_lock.unlock();
			this_thread::sleep_for(10ms);
		}
	}
}

int main()
{
	for (int i = 0; i < MAX_USER + 1; ++i) {
		auto& pl = objects[i];
		pl.id = i;
		pl.m_state = PLST_FREE;
		if (is_npc(i) == true) {
			sprintf_s(pl.m_name, "N%d", i);
			pl.m_state = PLST_INGAME;
			pl.x = rand() % WORLD_X_SIZE;
			pl.y = rand() % WORLD_Y_SIZE;
		}
	}

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	wcout.imbue(locale("korean"));
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(listenSocket), h_iocp, SERVER_ID, 0);
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
	listen(listenSocket, SOMAXCONN);

	EX_OVER accept_over;
	accept_over.m_op = OP_ACCEPT;
	memset(&accept_over.m_over, 0, sizeof(accept_over.m_over));
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	accept_over.m_csocket = c_socket;
	BOOL ret = AcceptEx(listenSocket, c_socket,
		accept_over.m_packetbuf, 0, 32, 32, NULL, &accept_over.m_over);
	if (FALSE == ret) {
		int err_num = WSAGetLastError();
		if (err_num != WSA_IO_PENDING)
			display_error("AcceptEx Error", err_num);
	}

	vector <thread> worker_threads;
	for (int i = 0; i < 1; ++i)
		worker_threads.emplace_back(do_worker, h_iocp, listenSocket);

	thread timer_thread{ do_timer };
	timer_thread.join();

	for (auto& th : worker_threads)
		th.join();
	closesocket(listenSocket);
	WSACleanup();
}
