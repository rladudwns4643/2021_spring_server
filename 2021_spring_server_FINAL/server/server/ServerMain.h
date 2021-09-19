#pragma once
#include "pch.h"
#include "server_define.h"
#include "Singleton.h"
#include "ThreadHandler.h"
#include "SocketUtil.h"
#include "extern.h"

class ServerMain :public Singleton<ServerMain> {
public:
	ServerMain();
	~ServerMain();

	void Initalize();
	void Run();

	void error_display(const char*, int);
	void AddTimer(EVENT& ev);

public:
	sockaddr_in GetServerAddr();

public:
	void SendPacket(UINT id, void* buf);

	void SendLoginOKPacket(UINT id);
	void SendLoginFailPacket(UINT id);
	void SendPositionPacket(UINT to_id, UINT info_id);
	void SendStatChangePacket(UINT to_id, UINT info_id);
	void SendChatPacket(UINT to_id, UINT info_id, char* chat, int chat_type);
	void SendAddObjectPacket(UINT to_id, UINT info_id);
	void SendRemoveObjectPacket(UINT to_id, UINT info_id);
	
private:
	ThreadHandler* m_ThreadHandler;
	SocketUtil m_sockUtil;
	TCPSocketPtr m_listen;

	sockaddr_in m_ServerAddr{};
};