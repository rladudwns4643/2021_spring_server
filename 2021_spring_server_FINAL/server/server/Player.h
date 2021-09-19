#pragma once
#include "pch.h"
#include "server_define.h"
#include "extern.h"
#include "ServerMain.h"

namespace NPC_TYPE {
	constexpr int Peace = 0;
	constexpr int Agro = 1;
};

class Player {
public:
	Player();
	Player(const Player& rhs);
	Player& operator=(const Player& rhs) noexcept;
	~Player();

public:
	void Initialize();
	virtual void Reset();

	//----------------------------------------------------
	void SetID(const UINT& id) { m_id = id; }
	int GetID() { return m_id; }

	void SetID_STR(char* name) { memcpy(&m_id_str, name, sizeof(char) * MAX_ID_LEN); }
	char* GetID_STR() { return m_id_str; }

	void SetPos(const POINT& pt) { m_pos = pt; }
	POINT GetPos() { return m_pos; }

	void SetExp(const UINT& exp) { m_exp = exp; }
	int GetExp() { return m_exp; }

	void SetHp(const UINT& hp) { m_hp = hp; }
	int GetHp() { return m_hp; }

	void SetLevel(const UINT& level) { m_level = level; }
	int GetLevel() { return m_level; }

	void SetEmpty(const bool& empty) { is_empty = empty;}
	bool GetEmpty() { return is_empty; }
	//----------------------------------------------------
	void SetisNPC(const bool& isNPC) { m_isNPC = isNPC; }
	bool GetisNPC() { return m_isNPC; }

	void SetNPCType(const int& type) { m_npc_type = type; }
	int GetNPCType() { return m_npc_type; }

	void SetMoveable(const bool& moveable) { m_moveable = moveable; }
	bool GetMoveable() { return m_moveable; }
	//----------------------------------------------------

	bool Update(float elapsedTime);
	void move(int dir);
	void move(POINT pt); //for teleport
	void attack(UINT target_id);
	void attack();
	void die();
	void revival();

protected:
	UINT m_id; // =g_users key val
	char m_id_str[MAX_ID_LEN];

	POINT m_pos;

	UINT m_level;
	UINT m_hp;
	UINT m_exp;

	bool is_empty;

private:
	std::uniform_int_distribution<int> uid_x{ 0, WORLD_WIDTH };
	std::uniform_int_distribution<int> uid_y{ 0, WORLD_HEIGHT };

protected:

	bool m_isNPC = true;
	int m_npc_type = NPC_TYPE::Peace;
	bool m_moveable = false;
	bool m_enable = false;
	SHORT NPC_ATK = 5;
public:
	UINT m_find_user_x;
	UINT m_find_user_y;
};