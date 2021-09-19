#pragma once
#include "pch.h"
#include "server_define.h"
#include "World.h"
#include "Player.h"
#include "DataBase.h"

class World;
namespace SR {
	extern HANDLE g_iocp;
	extern std::array<CLIENT*, MAX_USER> g_users;
	extern std::priority_queue<EVENT> g_timer_queue;

	extern World* g_world;
	extern Map* g_map;

	extern std::default_random_engine g_dre;

	extern DataBase* g_db;

}

namespace ATOMIC {
	extern std::mutex g_timer_lock;
	extern std::mutex g_user_lock;
	extern std::mutex g_map_lock;
	extern std::mutex g_msg_lock;
}