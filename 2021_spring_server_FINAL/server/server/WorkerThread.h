#pragma once
#include "pch.h"
#include "ThreadInterface.h"
#include "server_define.h"
#include "ServerMain.h"

class WorkerThread : public ThreadInterface {
public:
	void InitThread() override;
	void ProcThread() override;
	void JoinThread() override;
	message ProcPacket(UINT id, void* buf);

public:
	void DisconnectClient(UINT id);

private:
	SOCKADDR_IN m_ServerAddr;
	int m_AddrLen;
};