#include "pch.h"
#include "World.h"
#include "ServerMain.h"

World::World()
{
	Initialize();
}

World::~World()
{
}

void World::Initialize()
{
	m_lastUpdate = std::chrono::system_clock::now();

	EVENT ev_update{ GLOBAL_DEFINE::EVENT_KEY,std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(30), EV_UPDATE };
	ServerMain::GetInstance()->AddTimer(ev_update);
}

void World::Reset()
{
	if (this == nullptr) return;

	m_lastUpdate = std::chrono::system_clock::now();
	m_curPlayerNum = 0;
	m_map->MakeNewMap();
	for (int i = 0; i < MAX_USER; ++i) {
		delete SR::g_users[i]->cur; //������ �������
	}
}

void World::Update()
{
	if (this == nullptr) return;

	m_curUpdate = std::chrono::system_clock::now();
	m_elapsedSec = m_curUpdate - m_lastUpdate;
	m_lastUpdate = m_curUpdate;

	float elapsedMilliSec{ (m_elapsedSec.count() * 1000.f) };

	CopyRecvMsg();
	message procMsg;
	while (!m_copiedRecvMsgs.empty()) {
		procMsg = m_copiedRecvMsgs.front();
		m_copiedRecvMsgs.pop();
		ProcMsg(procMsg);
	}

	//player update
	for (int i = 0; i < MAX_USER; ++i) {
		if (SR::g_users[i] == nullptr) continue;
		if (SR::g_users[i]->isConnect == false) continue;
		int id{ SR::g_users[i]->cur->GetID() };
		if (id != -1) {
			SR::g_users[i]->cur->Update(elapsedMilliSec);
		}
	}

	++m_countFrame;

	std::chrono::system_clock::time_point updateEnd{ std::chrono::system_clock::now() };
	std::chrono::duration<float> updateTime{ updateEnd - m_curUpdate };

	//60fps
	int nextUpdate{ static_cast<int>((0.016f - updateTime.count()) * 1000.f) };
	if (nextUpdate <= 0) nextUpdate = 0;

	EVENT ev_update{ GLOBAL_DEFINE::EVENT_KEY, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(nextUpdate), EV_UPDATE };
	ServerMain::GetInstance()->AddTimer(ev_update);

	EVENT ev_flush{ GLOBAL_DEFINE::EVENT_KEY, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(1), EV_FLUSH_MSG };
	ServerMain::GetInstance()->AddTimer(ev_flush);
}

void World::PushMsg(message& msg)
{
	ATOMIC::g_msg_lock.lock();
	m_recvMsgQueue.push(msg);
	ATOMIC::g_msg_lock.unlock();
}

void World::PushSendMsg(UINT id, void* buf)
{
	char* p = reinterpret_cast<char*>(buf);
	SEND_INFO sinfo{};
	sinfo.to_id = id;
	memcpy(&sinfo.buf, buf, (BYTE)p[0]);

	m_sendMsgQueueLock.lock();
	m_sendMsgQueue.push(sinfo);
	m_sendMsgQueueLock.unlock();
}

void World::FlushSendMsg()
{
	if (this == nullptr) return;

	m_sendMsgQueueLock.lock();
	std::queue<SEND_INFO> q{ m_sendMsgQueue };
	while (!m_sendMsgQueue.empty()) {
		m_sendMsgQueue.pop();
	}
	m_sendMsgQueueLock.unlock();

	while (!q.empty()) {
		SEND_INFO sinfo = q.front();
		q.pop();

		ServerMain::GetInstance()->SendPacket(sinfo.to_id, &sinfo.buf);
	}
}

void World::CopyRecvMsg()
{
	m_recvMsgQueueLock.lock();
	m_copiedRecvMsgs = m_recvMsgQueue;
	m_recvMsgQueueLock.unlock();

	m_recvMsgQueueLock.lock();
	while (!m_recvMsgQueue.empty()) m_recvMsgQueue.pop();

	m_recvMsgQueueLock.unlock();
}

void World::ClearCopyMsg()
{
	while (!m_copiedRecvMsgs.empty()) m_copiedRecvMsgs.pop();
}

void World::ProcMsg(message msg)
{
	switch (msg.msg_type) {
	case CS_MOVE: {
		SR::g_users[msg.id]->cur->move(msg.dir);
		break;
	}
	case CS_ATTACK: {
		SR::g_users[msg.id]->cur->attack();
		break;
	}
	case CS_CHAT: {
		break;
	}
	case CS_LOGOUT: {
		break;
	}
	case CS_TELEPORT: {
		SR::g_users[msg.id]->cur->move(msg.pos);
		break;
	}
	default: {
		assert(!"unknown Msg");
	}
	}
}

bool World::isNear(int id1, int id2)
{
	const auto& id1x = SR::g_users[id1]->cur->GetPos().x;
	const auto& id1y = SR::g_users[id1]->cur->GetPos().y;
	const auto& id2x = SR::g_users[id2]->cur->GetPos().x;
	const auto& id2y = SR::g_users[id2]->cur->GetPos().y;
	if (abs(id1x - id2x) < VIEW_RADIUS && abs(id1y - id2y) < VIEW_RADIUS) {
		return true;
	}
	return false;

}

bool World::isNpc(int id)
{
	return id >= NPC_ID_START;
}

bool World::EnterPlayer(int id) {
	if (m_curPlayerNum >= MAX_USER) return false;

	ATOMIC::g_user_lock.lock();
	for (int i = 0; i < MAX_USER; ++i) {
		if (SR::g_users[i] == nullptr) continue;
		if (SR::g_users[i]->cur->GetEmpty() == false) {
			++m_curPlayerNum;
			SR::g_users[i]->cur->SetEmpty(false);
			break;
		}
	}
	ATOMIC::g_user_lock.unlock();
	return true;
}

void World::wake_up_npc(int id)
{
	bool flag = false;
	if (true == SR::g_users[id]->isActive.compare_exchange_strong(flag, true) && SR::g_users[id]->isAlive == true) {
		EVENT ev{ id, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(1000), EV_RANDOM_MOVE };
		ServerMain::GetInstance()->AddTimer(ev);
	}
}

void World::random_move_npc(int id) {
	int target_id = -1;
	std::unordered_set<int> old_viewlist;
	for (int i = 0; i < NPC_ID_START; ++i) {
		if (SR::g_users[i] == nullptr) continue;
		if (SR::g_users[i]->isConnect == false) continue;
		if (isNear(id, i)) {
			if (isNpc(i) == false && SR::g_users[i]->cur->GetEmpty() == false) {
				target_id = i;
			}
			old_viewlist.insert(i);
		}
	}

	const auto& my_x = SR::g_users[id]->cur->GetPos().x;
	const auto& my_y = SR::g_users[id]->cur->GetPos().y;

	
	std::uniform_int_distribution<int> uid{ 0, 3 };

	if (target_id == -1) {
		SR::g_users[id]->cur->move(uid(SR::g_dre));
	}
	else {
		const auto& target_x = SR::g_users[target_id]->cur->GetPos().x;
		const auto& target_y = SR::g_users[target_id]->cur->GetPos().y;
		// FIND 11*11(aStar)
		if (abs(my_x - target_x) < 11 && abs(my_y - target_y) < 11) {
			std::vector<POINT> aStar = AStar(target_x, target_y, my_x, my_y);
			if (!aStar.empty() && aStar.size() > 2) {
				POINT t{ aStar[aStar.size() - 2] };
				SR::g_users[id]->cur->SetPos(t);
			}
		}

		//attack
		if (abs(my_x - target_x) < 2 && abs(my_y - target_y) < 2) {
			SR::g_users[id]->cur->attack(target_id);
			ServerMain::GetInstance()->SendStatChangePacket(target_id, target_id);
		}
	}
	std::unordered_set<int> new_viewlist;
	for (int i = 0; i < NPC_ID_START; ++i) {
		if (SR::g_users[i] == nullptr) continue;
		if (id == i) continue;
		if (SR::g_users[i]->isConnect == false) continue;
		if (isNear(id, i) == false) continue;
		new_viewlist.insert(i);
	}

	for (auto& p : old_viewlist) {
		if (new_viewlist.count(p) > 0) {
			if (SR::g_users[p]->viewlist.count(id) > 0) {
				ServerMain::GetInstance()->SendPositionPacket(p, id);
			}
			else {
				SR::g_users[p]->viewlist.insert(id);
				ServerMain::GetInstance()->SendAddObjectPacket(p, id);
			}
		}
		if (new_viewlist.count(p) == 0) {
			if (SR::g_users[p]->viewlist.count(id) > 0) {
				SR::g_users[p]->viewlist.erase(id);
				ServerMain::GetInstance()->SendRemoveObjectPacket(p, id);
			}
		}
	}

	for (auto& p : new_viewlist) {
		if (SR::g_users[p]->viewlist.count(p) == 0) {
			if (SR::g_users[p]->viewlist.count(id) == 0) {
				SR::g_users[p]->viewlist.insert(id);
				ServerMain::GetInstance()->SendAddObjectPacket(p, id);
			}
			else {
				ServerMain::GetInstance()->SendPositionPacket(p, id);
			}
		}
	}

}

bool operator < (const POINT& lhs, const POINT& rhs) {
	if (lhs.x == rhs.x) return lhs.y < rhs.y;
	return lhs.x < rhs.x;
}

bool operator <(const Node& lhs, const Node& rhs) {
	return false;
}

bool operator < (const Huristic& lhs, const Huristic& rhs) {
	return (lhs.movingDist + lhs.preDist) < (rhs.movingDist + rhs.preDist);
}

std::vector<POINT> World::AStar(int targetX, int targetY, int posx, int posy)
{
	int dx[] = { 0, -1, 1, 0 };
	int dy[] = { -1, 0, 0, 1 };

	// resultPath: ���������� �̵��� ��θ� �����ϴ°�
	std::vector<POINT> resultPath;

	// openQ: ���ɼ� �ִ� ��带 ã�´�.(�޸���ƽ ����ġ �������� ���ĵ�����)
	// closeQ: Ž�� ��ο��� ������ �湮���� �ִ� ������ �����ص�.
	std::priority_queue<std::pair<Huristic, Node>, std::vector<std::pair<Huristic, Node>>, std::greater<std::pair<Huristic, Node>>> openQ;
	std::map<POINT, POINT> closeQ; // key: ������ġ, value: ������ġ

	// ť�� �� ��ġ�� �ְ� �����Ѵ�.
	std::pair<Huristic, Node> first;

	first.second.curLocation = { posx,posy };
	first.second.prevLocation = { -1,-1 };
	first.first.movingDist = 0;
	first.first.preDist = -1;

	openQ.push(first);

	// AStarAlgorithmCount: BFSŽ���� ���ϱ� ���ؼ� ���� ��θ� ã�������� �����ݺ��� Ƚ���� �����Ѵ�.
	int AStarAlgorithmCount = 0;

	// ���ɼ��ִ� ��尡 ���������� �ݺ��Ѵ�.

	while (!openQ.empty())
	{
		AStarAlgorithmCount++;

		Node currentNode = openQ.top().second;
		Huristic currentHuristic = openQ.top().first;
		openQ.pop();

		if (closeQ.find(currentNode.curLocation) != closeQ.end())
			continue;

		closeQ.insert({ currentNode.curLocation, currentNode.prevLocation });

		int x = currentNode.curLocation.x;
		int y = currentNode.curLocation.y;
		int nextY = x;
		int nextX = y;
		// �������� ���������� �ݺ��� ����

		if (x == targetX && y == targetY)
			break;

		for (int i = 0; i < 4; i++)
		{
			// ������ġ�� ��ü���� ������ ��
			nextY = y + dy[i];
			nextX = x + dx[i];
			//if (mapInfo[nextY][nextX] != NO_OBSTACLE_MAP) return
			if (nextX >= 0 && nextX < WORLD_WIDTH && nextY >= 0 && nextY < WORLD_HEIGHT)
			{
				// ���� �ƴϰų�, ������ �湮�Ѱ��� ���� �ش� ��带 �����Ѵ�.
				if (SR::g_map->getData(nextX,nextY) != GRID_TYPE::eBLOCKED)
					//||closeQ.find({ nextX,nextY }) != closeQ.end())
					continue;

				Huristic huristic;
				Node nextNode;

				//cout << nextY << "," << nextX << endl;
				// ������ġ�� ��ǥ��ġ���� �����Ÿ��� ���ؼ� �޸���ƽ����ġ�� Ȱ��
				int deltaX = nextX - targetX;
				int deltaY = nextY - targetY;
				float directDist = sqrt(deltaX * deltaX + deltaY * deltaY);

				huristic.movingDist = currentHuristic.movingDist + 1;
				huristic.preDist = directDist;

				nextNode.curLocation.x = nextX;
				nextNode.curLocation.y = nextY;
				nextNode.prevLocation.x = x;
				nextNode.prevLocation.y = y;

				openQ.push({ huristic, nextNode });
			}
		}

	}

	// ���������� ���� ��ΰ� ������ �� ������Ϳ� �ִ´�.
	if (closeQ.find(POINT{ targetX, targetY }) != closeQ.end())
	{
		POINT location = { targetX, targetY };
		while (location.x != -1)
		{
			resultPath.push_back(location);
			location = closeQ[location];
		}
	}

	return resultPath;
}