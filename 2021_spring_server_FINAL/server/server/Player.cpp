#include "Player.h"

Player::Player()
{
	Initialize();
}

Player::Player(const Player& rhs)
{
	if (this != &rhs) {
		m_id = rhs.m_id;
		memcpy(m_id_str, rhs.m_id_str, sizeof(char) * MAX_ID_LEN);
		m_pos = rhs.m_pos;
	}
}

Player& Player::operator=(const Player& rhs) noexcept
{
	if (this != &rhs) {
		m_id = rhs.m_id;
		memcpy(m_id_str, rhs.m_id_str, sizeof(char) * MAX_ID_LEN);		
		m_pos = rhs.m_pos;
	}
	return *this;
}

Player::~Player(){}

void Player::Initialize()
{
	Reset();
	//do {
	//	m_pos.x = uid_x(SR::g_dre);
	//	m_pos.y = uid_y(SR::g_dre);
	//} while (SR::g_map->getData(m_pos.x, m_pos.y) != 0);
}

void Player::Reset()
{
	m_id = -1;
	ZeroMemory(&m_id_str, sizeof(char) * MAX_ID_LEN);
	m_pos.x = -1;
	m_pos.y = -1;
	m_enable = false;
}

bool Player::Update(float elapsedTime)
{
	//std::cout << "call player update\n";
	return true;
}

void Player::move(int dir) {
	auto px = m_pos.x;
	auto py = m_pos.y;
	auto x = m_pos.x;
	auto y = m_pos.y;

	ATOMIC::g_map_lock.lock();
	switch (dir) {
	case DIR_TYPE::MV_UP: {
		if (y > 0 && SR::g_map->data[x][y - 1] == GRID_TYPE::eBLANK) --y;
		break;
	}
	case DIR_TYPE::MV_DOWN: {
		if (y < WORLD_HEIGHT && SR::g_map->data[x][y + 1] == GRID_TYPE::eBLANK) ++y;
		break;
	}
	case DIR_TYPE::MV_LEFT: {
		if (x > 0 && SR::g_map->data[x - 1][y] == GRID_TYPE::eBLANK) --x;
		break;
	}
	case DIR_TYPE::MV_RIGHT: {
		if (x < WORLD_WIDTH && SR::g_map->data[x + 1][y] == GRID_TYPE::eBLANK) ++x;
		break;
	}
	default: {
		assert(!"Player::move");
	}
	}

	SR::g_map->setData(px, py, GRID_TYPE::eBLANK);
	SR::g_map->setData(x, y, GRID_TYPE::ePLAYER);
	ATOMIC::g_map_lock.unlock();

	std::unordered_set<int>old_viewlist = SR::g_users[m_id]->viewlist;
	ServerMain::GetInstance()->SendPositionPacket(m_id, m_id);

	std::unordered_set<int> new_viewlist;
	for (int i = 0; i < NPC_ID_START; ++i) {
		if (SR::g_users[i] == nullptr) continue;
		if (m_id == i) continue;
		if (SR::g_users[i]->isConnect == false) continue;
		if (SR::g_world->isNear(m_id, i) == true) {
			new_viewlist.insert(i);
		}
	}
	for (int i = NPC_ID_START; i < MAX_USER; ++i){
		if (SR::g_world->isNpc(i) == false) continue;
		if (SR::g_world->isNear(m_id, i) == true) {
			new_viewlist.insert(i);
		}
		SR::g_world->wake_up_npc(i);
		ServerMain::GetInstance()->SendStatChangePacket(m_id, i);
	}

	for (auto& n : new_viewlist) {
		if (old_viewlist.count(n) == 0) { //전에는 없고, 후에는 있음
			SR::g_users[m_id]->viewlist.insert(n);
			ServerMain::GetInstance()->SendAddObjectPacket(m_id, n);

			if (SR::g_world->isNpc(n) == true) {
				ServerMain::GetInstance()->SendPositionPacket(n, m_id);
			}
			else {
				if (SR::g_users[n]->viewlist.count(m_id) == 0) { //상대쪽에도 없으면 나 추가
					SR::g_users[n]->viewlist.insert(m_id);
					ServerMain::GetInstance()->SendAddObjectPacket(m_id, n);
				}
			}
		}
		else { //전에도, 후에도 있음
			if (SR::g_world->isNpc(n) == true) {
				SR::g_users[n]->viewlist.insert(n);

				ServerMain::GetInstance()->SendAddObjectPacket(n, m_id);
			}
			else {
				SR::g_users[n]->viewlist_lock.lock();
				if (SR::g_users[n]->viewlist.count(m_id)) { //상대쪽에도 있으면 나 이동 전달
					ServerMain::GetInstance()->SendPositionPacket(n, m_id);
				}
				SR::g_users[n]->viewlist_lock.unlock();
			}
		}
	}
	for (auto& o : old_viewlist) {
		if (new_viewlist.count(o) == 0) {
			SR::g_users[m_id]->viewlist.erase(o);
			ServerMain::GetInstance()->SendRemoveObjectPacket(m_id, o);
			if (SR::g_world->isNpc(o) == false) {
				if (SR::g_users[o]->viewlist.count(m_id)) {
					SR::g_users[o]->viewlist.erase(m_id);
					ServerMain::GetInstance()->SendRemoveObjectPacket(o, m_id);
				}
			}
		}
	}

	if (SR::g_world->isNpc(m_id) == false) {
		for (auto& npc : new_viewlist) {
			if (SR::g_world->isNpc(npc) == false) continue;
			EVENT ev{ npc, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(1), EV_PLAYER_MOVE_NOTIFY, m_id };
			ServerMain::GetInstance()->AddTimer(ev);
		}
	}
}

void Player::move(POINT pt)
{
	auto& x = m_pos.x;
	auto& y = m_pos.y;
	ATOMIC::g_map_lock.lock();
	SR::g_map->setData(x, y, GRID_TYPE::eBLANK);
	x = pt.x;
	y = pt.y;
	SR::g_map->setData(x, y, GRID_TYPE::ePLAYER);
	ATOMIC::g_map_lock.unlock();

	std::unordered_set<int>old_viewlist = SR::g_users[m_id]->viewlist;
	ServerMain::GetInstance()->SendPositionPacket(m_id, m_id);

	std::unordered_set<int> new_viewlist;
	for (int i = 0; i < NPC_ID_START; ++i) {
		if (SR::g_users[i] == nullptr) continue;
		if (m_id == i) continue;
		if (SR::g_users[i]->isConnect == false) continue;
		if (SR::g_world->isNear(m_id, i) == false) continue;
		new_viewlist.insert(i);
	}
	for (int i = NPC_ID_START; i < MAX_USER; ++i) {
		if (SR::g_world->isNear(m_id, i) == false) continue;
		if (SR::g_world->isNpc(i) == false) continue;
		new_viewlist.insert(i);
		SR::g_world->wake_up_npc(i);
		ServerMain::GetInstance()->SendStatChangePacket(m_id, i);
	}

	for (auto& n : new_viewlist) {
		if (old_viewlist.count(n) == 0) { //전에는 없고, 후에는 있음
			SR::g_users[m_id]->viewlist.insert(n);
			ServerMain::GetInstance()->SendAddObjectPacket(m_id, n);

			if (SR::g_world->isNpc(n) == true) {
				ServerMain::GetInstance()->SendPositionPacket(n, m_id);
			}
			else {
				if (SR::g_users[n]->viewlist.count(m_id) == 0) { //상대쪽에도 없으면 나 추가
					SR::g_users[n]->viewlist.insert(m_id);
					ServerMain::GetInstance()->SendAddObjectPacket(m_id, n);
				}
			}
		}
		else { //전에도, 후에도 있음
			if (SR::g_world->isNpc(n) == true) {
				SR::g_users[n]->viewlist.insert(n);
				ServerMain::GetInstance()->SendAddObjectPacket(n, m_id);
			}
			else {
				if (SR::g_users[n]->viewlist.count(m_id)) { //상대쪽에도 있으면 나 이동 전달
					ServerMain::GetInstance()->SendPositionPacket(n, m_id);
				}
			}
		}
	}
	for (auto& o : old_viewlist) {
		if (new_viewlist.count(o) == 0) {
			SR::g_users[m_id]->viewlist.erase(o);
			ServerMain::GetInstance()->SendRemoveObjectPacket(m_id, o);
			if (SR::g_world->isNpc(o) == false) {
				if (SR::g_users[o]->viewlist.count(m_id)) {
					SR::g_users[o]->viewlist.erase(m_id);
					ServerMain::GetInstance()->SendRemoveObjectPacket(o, m_id);
				}
			}
		}
	}
	if (SR::g_world->isNpc(m_id) == false) {
		for (auto& npc : new_viewlist) {
			if (SR::g_world->isNpc(npc) == false) continue;
			EVENT ev{ npc, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(1), EV_PLAYER_MOVE_NOTIFY, m_id };
			ServerMain::GetInstance()->AddTimer(ev);
		}
	}

}

void Player::attack(UINT target_id)
{
	int left = SR::g_users[target_id]->cur->GetHp() - NPC_ATK;
	SR::g_users[target_id]->cur->SetHp(left);
	if (left <= 0) {
		SR::g_users[target_id]->cur->die();
	}
}

void Player::attack() {
	auto& u = SR::g_users[m_id];
	SR::g_users[m_id]->viewlist_lock.lock();
	auto vl = u->viewlist;
	SR::g_users[m_id]->viewlist_lock.unlock();

	const auto& my_x = u->cur->GetPos().x;
	const auto& my_y = u->cur->GetPos().y;

	char chat[MAX_CHAT_LEN];
	for (auto& npc : vl) {
		auto& n = SR::g_users[npc];
		const auto& target_x = n->cur->GetPos().x;
		const auto& target_y = n->cur->GetPos().y;

		if (abs(my_x - target_x) < 2 && abs(my_y - target_y) < 2) {
			n->cur->SetHp(GetHp() - (u->cur->GetLevel() * 2));

			//npc 죽었으면 경험치
			if (n->cur->GetHp() < 0 && n->isActive == true) {
				if (n->isAlive == true) {
					u->cur->SetExp(u->cur->GetExp() + n->cur->GetLevel() * n->cur->GetLevel() * 2);
					if (u->cur->GetExp() >= (100 * pow(2, (u->cur->GetLevel() - 1)))) {
						//level up
						u->cur->SetExp(0);
						u->cur->SetHp(100);
						u->cur->SetLevel(u->cur->GetLevel() + 1);
					}
				}
				n->isAlive = false;
				n->isActive = false;

				sprintf_s(chat, "ID: %s / EXP: %d / LEVEL: %d", u->cur->GetID_STR(), u->cur->GetExp(), u->cur->GetLevel());

				ServerMain::GetInstance()->SendChatPacket(m_id, m_id, chat, 0);
				ServerMain::GetInstance()->SendStatChangePacket(m_id, npc);

				for (auto& vl : u->viewlist) {
					ServerMain::GetInstance()->SendStatChangePacket(vl, npc);
				}
			}
			
		}
	}
}

void Player::die() {
	if (m_hp < 0) {
		m_hp = 100;
		m_exp /= 2;

		char chat[MAX_CHAT_LEN];
		sprintf_s(chat, "ID: %s / EXP: %d / LEVEL: %d", m_id_str, m_exp, m_level);
		revival();

		ServerMain::GetInstance()->SendChatPacket(m_id, m_id, chat, 0);
	}
}

void Player::revival() {
	std::uniform_int_distribution<int> uid{ 0, 9 };
	m_pos = REVIVAL_POINT::pt[uid(SR::g_dre)];
	ServerMain::GetInstance()->SendPositionPacket(m_id, m_id);
}