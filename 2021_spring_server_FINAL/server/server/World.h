#pragma once
#include "Map.h"
#include "server_define.h"
#include "Player.h"

class Player;
class World {
public:
	explicit World();
	~World();

public:
	void Initialize();
	void Reset();
	void Update();
	
public:
	void PushMsg(message& msg);
	void PushSendMsg(UINT id, void* buf);
	void FlushSendMsg();

public:
	void CopyRecvMsg();
	void ClearCopyMsg();
	void ProcMsg(message msg);

public:
	bool isNear(int id1, int id2);
	bool isNpc(int id);
	bool EnterPlayer(int id);
	void wake_up_npc(int id);
	void random_move_npc(int id);
	std::vector<POINT> AStar(int targetX, int targetY, int posx, int posy);

	//bool is_start = false;
	//std::array<Player*, MAX_USER> m_players;
	std::atomic_int m_curPlayerNum;
private:

	std::queue<message> m_recvMsgQueue;
	std::queue<message> m_copiedRecvMsgs;
	std::mutex m_recvMsgQueueLock;

	std::queue<SEND_INFO> m_sendMsgQueue;
	std::mutex m_sendMsgQueueLock;

	Map* m_map = nullptr;

	std::chrono::system_clock::time_point m_lastUpdate;
	std::chrono::system_clock::time_point m_curUpdate;
	std::chrono::duration<float> m_elapsedSec;

	std::atomic_int m_countFrame{ 0 };

};