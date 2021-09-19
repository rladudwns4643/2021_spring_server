#include "pch.h"
#include "WorkerThread.h"

void WorkerThread::InitThread() {
	m_ServerAddr = ServerMain::GetInstance()->GetServerAddr();
	m_AddrLen = sizeof(m_ServerAddr);
	mythread = std::thread([&]() {WorkerThread::ProcThread(); });
}

void WorkerThread::ProcThread() {
	while (true) {
		DWORD num_byte;
		ULONG key;
		PULONG p_key = &key;
		WSAOVERLAPPED* p_over;

		bool bSuccess = GetQueuedCompletionStatus(SR::g_iocp, &num_byte, (PULONG_PTR)p_key, &p_over, INFINITE);
		if (!bSuccess) {
			int err = GetLastError();
			ServerMain::GetInstance()->error_display("GQCS ERROR: ", err);
		}

		SOCKET s;
		int ev = static_cast<int>(key);
		if (ev == GLOBAL_DEFINE::EVENT_KEY);
		else {
			if (SR::g_world->isNpc(key) == true);
			else {
				if (SR::g_users[key]->m_s == nullptr) {
					assert(!"use nullptr socket");
					continue;
				}

				s = SR::g_users[key]->m_s->GetSocket();
				if (num_byte == 0) {
					DisconnectClient(key);
					continue;
				}
			}
		}
		EX_OVER* ex_over = reinterpret_cast<EX_OVER*>(p_over);

		auto& user = SR::g_users[key];
		switch (ex_over->ev_type) {
		case EV_RECV: {
			char* buf = user->m_recv_over.net_buf;
			UINT psize = user->curr_packet_size;
			UINT pr_size = user->prev_packet_data;
			message msg;
			memset(&msg, 0, sizeof(message));
			msg.id = -1;
			msg.msg_type = GLOBAL_DEFINE::NO_MSG;
			while (num_byte > 0) {
				if (psize == 0) psize = (BYTE)buf[0];
				if (num_byte + pr_size >= psize) {
					unsigned char p[MAX_BUFFER];
					memcpy(p, user->packet_buf, pr_size);
					memcpy(p + pr_size, buf, psize - pr_size);
					msg = ProcPacket(key, p);
					if (msg.msg_type != GLOBAL_DEFINE::NO_MSG) {
						SR::g_world->PushMsg(msg);
					}
					num_byte -= psize - pr_size;
					buf += psize - pr_size;
					psize = 0;
					pr_size = 0;
				}
				else {
					ATOMIC::g_user_lock.lock();
					memcpy(user->packet_buf + pr_size, buf, num_byte);
					ATOMIC::g_user_lock.unlock();
					pr_size += num_byte;
					num_byte = 0;
				}
			}
			user->curr_packet_size = psize;
			user->prev_packet_data = pr_size;

			DWORD flags = 0;
			ZeroMemory(&ex_over->over, sizeof(WSAOVERLAPPED));
			ZeroMemory(&msg, sizeof(message));
			ex_over->ev_type = EV_RECV;

			int ret = WSARecv(s, ex_over->wsabuf, 1, 0, &flags, &ex_over->over, 0);
			if (ret == SOCKET_ERROR) {
				int err_no = WSAGetLastError();
				if (err_no != ERROR_IO_PENDING) {
					ServerMain::GetInstance()->error_display("WSARecv Error", err_no);
					while (1);
				}
			}
			break;
		}
		case EV_SEND: {
			delete ex_over;
			break;
		}
		case EV_UPDATE: {
			SR::g_world->Update();
			delete ex_over;
			break;
		}
		case EV_FLUSH_MSG: {
			SR::g_world->FlushSendMsg();
			delete ex_over;
			break;
		}
		case EV_RANDOM_MOVE: {
			if (SR::g_users[key]->isAlive == true) {
				SR::g_world->random_move_npc(key);
				EVENT ev{ key, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(1000), EV_RANDOM_MOVE };
				ServerMain::GetInstance()->AddTimer(ev);
			}
			delete ex_over;
			break;
		}
		case EV_PLAYER_MOVE_NOTIFY: {
			SR::g_users[key]->lua_lock.lock();
			lua_State* L = SR::g_users[key]->L;
			lua_getglobal(L, "event_player_move"); //같은 위치면 "!" 띄우기
			lua_pushnumber(L, ex_over->obj_id);
			int err = lua_pcall(L, 1, 0, 0);
			SR::g_users[key]->lua_lock.unlock();
			delete ex_over;
			break;
		}
		default: {
			//assert(!"unknown event type");
		}
		}
	}
}

void WorkerThread::JoinThread() {
	mythread.join();
}
static int x;
static int y;
message WorkerThread::ProcPacket(UINT id, void* buf) {
	char* inputPacket = reinterpret_cast<char*>(buf);
	message msg;
	memset(&msg, 0, sizeof(message));
	msg.id = id;
	msg.msg_type = GLOBAL_DEFINE::NO_MSG;

	switch (inputPacket[1]) {
	case CS_LOGIN: {
		cs_packet_login* p = reinterpret_cast<cs_packet_login*>(inputPacket);
		std::cout << "CS_LOGIN\n";
		memcpy(&SR::g_users[id]->id_str, &p->player_id, sizeof(char) * MAX_ID_LEN);
		
		short x = -1, y = -1, hp;
		int exp, level;
		SR::g_db->FindID(SR::g_users[id]->id_str, x, y, hp, level, exp);
		if (x == -1 || y == -1) {
			ServerMain::GetInstance()->SendLoginFailPacket(id);
			break;
		}
		//load db
		SR::g_users[id]->user_id = id;
		SR::g_users[id]->cur->SetLevel(level);
		SR::g_users[id]->cur->SetHp(hp);
		SR::g_users[id]->cur->SetExp(exp);
		SR::g_users[id]->cur->SetPos(POINT{ x, y });
		//~load db
		SR::g_users[id]->cur->SetEmpty(true);

		SR::g_world->EnterPlayer(id);
		ServerMain::GetInstance()->SendLoginOKPacket(id);

		for (int i = 0; i < NPC_ID_START; ++i) {
			if (SR::g_users[i] == nullptr) continue;
			if (SR::g_users[i]->isConnect == true) {
				if (id == i) continue;
				if (SR::g_world->isNear(i, id) == true) {
					if (SR::g_users[i]->viewlist.count(id) == 0) {
						SR::g_users[i]->viewlist.insert(id);
						ServerMain::GetInstance()->SendAddObjectPacket(i, id);
					}
					if (SR::g_users[id]->viewlist.count(i) == 0) {
						SR::g_users[id]->viewlist.insert(i);
						ServerMain::GetInstance()->SendAddObjectPacket(id, i);
					}
				}
			}
		}

		for (int i = NPC_ID_START; i < MAX_USER; ++i) {
			if (SR::g_world->isNear(id, i) == true) {
				SR::g_users[id]->viewlist.insert(i);
				ServerMain::GetInstance()->SendAddObjectPacket(id, i);
				SR::g_world->wake_up_npc(i);
				ServerMain::GetInstance()->SendStatChangePacket(id, i);
			}
		}
		break;
	}
	case CS_DUMMY_LOGIN: {
		ServerMain::GetInstance()->SendLoginOKPacket(id);
		SR::g_users[id]->cur->SetPos(POINT{ x, y });
		x += 50;
		if (x > WORLD_WIDTH) x = 0;
		y += 50;
		if (y > WORLD_WIDTH) y = 0;
		SR::g_world->EnterPlayer(id);
		break;
	}
	case CS_MOVE: {
		cs_packet_move* p = reinterpret_cast<cs_packet_move*>(inputPacket);
		//std::cout << "CS_MOVE: " << (int)p->direction << "\n";
		msg.dir = (int)p->direction;
		SR::g_users[id]->move_time = p->move_time;
		msg.msg_type = CS_MOVE;
		break;
	}
	case CS_ATTACK: {
		cs_packet_attack* p = reinterpret_cast<cs_packet_attack*>(inputPacket);
		//std::cout << "CS_ATTACK\n";
		msg.msg_type = CS_ATTACK;
		break;
	}
	case CS_CHAT: {
		cs_packet_chat* p = reinterpret_cast<cs_packet_chat*>(inputPacket);
		//std::cout << "CS_CHAT\n";
		break;
	}
	case CS_LOGOUT: {
		cs_packet_logout* p = reinterpret_cast<cs_packet_logout*>(inputPacket);
		
		SR::g_db->UpdateDB(SR::g_users[id]->id_str, SR::g_users[id]->cur->GetPos().x, SR::g_users[id]->cur->GetPos().y,
			SR::g_users[id]->cur->GetHp(), SR::g_users[id]->cur->GetLevel(), SR::g_users[id]->cur->GetExp());

		SR::g_users[id]->isConnect = false;
		SR::g_users[id]->cur->Reset();
		break;
	}
	case CS_TELEPORT: {
		cs_packet_teleport* p = reinterpret_cast<cs_packet_teleport*>(inputPacket);
		std::cout << "CS_TELEPORT\n";
		break;
	}
	}
	//msg 처리할 거 전달
	return msg;
}

void WorkerThread::DisconnectClient(UINT id) {
	ATOMIC::g_user_lock.lock();
	SR::g_users[id]->isConnect = false;
	ZeroMemory(SR::g_users[id], sizeof(CLIENT));
	SR::g_users[id]->m_s = nullptr;
	ATOMIC::g_user_lock.unlock();
}