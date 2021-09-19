#include "ServerMain.h"

ServerMain::ServerMain()
{
	Initalize();
}

ServerMain::~ServerMain()
{
	m_ThreadHandler->JoinThreads();

	for (int i = 0; i < MAX_USER; ++i) {
		delete SR::g_users[i];
		SR::g_users[i] = nullptr;
	}
	m_sockUtil.CleanUp();
}

void ServerMain::Initalize()
{
	if (!m_sockUtil.StaticInit()) assert(!"ServerMain::Initalize()");

	SR::g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	m_listen = m_sockUtil.CreateTCPSocket(SocketAddressFamily::INET);
	SocketAddress addr{ INADDR_ANY, SERVER_PORT };
	m_listen->Bind(addr);
	m_listen->Listen();
}

void ServerMain::Run()
{
	m_ThreadHandler = new ThreadHandler;

	m_ThreadHandler->CreateThreads();

	SocketAddress addr;
	TCPSocketPtr s;
	DWORD flags = 0;

	while (true) {
		s = m_listen->Accept(addr);

		int user_id = -1;
		for (int i = 0; i < NPC_ID_START; ++i) {
			if (SR::g_users[i] == nullptr) continue;
			if (!SR::g_users[i]->isConnect) {
				user_id = i;
				break;
			}
		}
		if (user_id == -1) assert(!"ServerMain::Run no more slot");

		CLIENT* new_client = new CLIENT;
		new_client->user_id = user_id;
		new_client->m_s = s;
		new_client->m_recv_over.wsabuf[0].len = MAX_BUFFER;
		new_client->m_recv_over.wsabuf[0].buf = new_client->m_recv_over.net_buf;

		new_client->m_recv_over.ev_type = EV_RECV;
		new_client->isConnect = true;

		new_client->curr_packet_size = 0;
		new_client->prev_packet_data = 0;

		new_client->cur = new Player;
		new_client->cur->SetID(user_id);
		new_client->cur->SetEmpty(true);

		SR::g_users[user_id] = new_client;

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(s->GetSocket()), SR::g_iocp, user_id, 0);
		memset(&SR::g_users[user_id]->m_recv_over, 0, sizeof(WSAOVERLAPPED));
		s->WSAReceive(SR::g_users[user_id]->m_recv_over.wsabuf, 1, NULL, &flags, &SR::g_users[user_id]->m_recv_over.over);
		s = nullptr;
	}
}

void ServerMain::error_display(const char* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"error: " << err_no << lpMsgBuf << std::endl;
	//while (true); //error
	LocalFree(lpMsgBuf);
}

void ServerMain::AddTimer(EVENT& ev)
{
	ATOMIC::g_timer_lock.lock();
	SR::g_timer_queue.push(ev);
	ATOMIC::g_timer_lock.unlock();
}

sockaddr_in ServerMain::GetServerAddr()
{
	return m_ServerAddr;
}

void ServerMain::SendPacket(UINT id, void* buf)
{
	char* p = reinterpret_cast<char*>(buf);
	BYTE packet_size = (BYTE)p[0];
#ifdef LOG_ON
	cout << "[main]: Send: " << (int)p[1] << " id: " << id << endl;
#endif //LOG_ON
	EX_OVER* send_over = new EX_OVER;
	ZeroMemory(send_over, sizeof(EX_OVER));
	send_over->ev_type = EV_SEND;
	memcpy(send_over->net_buf, p, packet_size);
	send_over->wsabuf[0].len = packet_size;
	send_over->wsabuf[0].buf = send_over->net_buf;

	SR::g_users[id]->m_s->WSASend(send_over->wsabuf, 1, 0, 0, &send_over->over);
}

void ServerMain::SendLoginOKPacket(UINT id)
{
	sc_packet_login_ok p;
	p.size = sizeof(p);
	p.type = SC_LOGIN_OK;
	p.id = id;
	p.HP = SR::g_users[id]->cur->GetHp();
	p.EXP = SR::g_users[id]->cur->GetExp();
	p.LEVEL = SR::g_users[id]->cur->GetLevel();
	p.x = SR::g_users[id]->cur->GetPos().x;
	p.y = SR::g_users[id]->cur->GetPos().y;
	SendPacket(id, &p);
}

void ServerMain::SendLoginFailPacket(UINT id) {
	sc_packet_login_fail p;
	p.size = sizeof(p);
	p.type = SC_LOGIN_FAIL;
	SendPacket(id, &p);
}

void ServerMain::SendPositionPacket(UINT to_id, UINT info_id)
{
	sc_packet_position p;
	p.size = sizeof(p);
	p.type = SC_POSITION;
	p.id = info_id;
	p.move_time = SR::g_users[info_id]->move_time;
	p.x = SR::g_users[info_id]->cur->GetPos().x;
	p.y = SR::g_users[info_id]->cur->GetPos().y;
	SendPacket(to_id, &p);
}

void ServerMain::SendStatChangePacket(UINT to_id, UINT info_id) {
	sc_packet_stat_change p;
	p.size = sizeof(p);
	p.type = SC_STAT_CHANGE;
	p.id = info_id;
	p.HP = SR::g_users[info_id]->cur->GetHp();
	p.EXP = SR::g_users[info_id]->cur->GetExp();
	p.LEVEL = SR::g_users[info_id]->cur->GetLevel();
	SendPacket(to_id, &p);
}

void ServerMain::SendChatPacket(UINT to_id, UINT info_id, char* chat, int chat_type)
{
	sc_packet_chat p;
	p.id = info_id;
	p.size = sizeof(p);
	p.type = SC_CHAT;
	strcpy_s(p.message, chat);
	SendPacket(to_id, &p);
}

void ServerMain::SendAddObjectPacket(UINT to_id, UINT info_id)
{
	sc_packet_add_object p;
	p.size = sizeof(p);
	p.type = SC_ADD_OBJECT;
	p.id = info_id;
	p.HP = SR::g_users[info_id]->cur->GetHp();
	p.EXP = SR::g_users[info_id]->cur->GetExp();
	p.LEVEL = SR::g_users[info_id]->cur->GetLevel();
	p.x = SR::g_users[info_id]->cur->GetPos().x;
	p.y = SR::g_users[info_id]->cur->GetPos().y;
	strcpy_s(p.name, SR::g_users[info_id]->cur->GetID_STR());
	p.obj_class = SR::g_users[info_id]->cur->GetNPCType();
	SendPacket(to_id, &p);
}

void ServerMain::SendRemoveObjectPacket(UINT to_id, UINT info_id)
{
	sc_packet_remove_object p;
	p.size = sizeof(p);
	p.type = SC_REMOVE_OBJECT;
	p.id = info_id;
	SendPacket(to_id, &p);
}
