#include "pch.h"
#include "server_define.h"
#include "World.h"
#include "Map.h"
#include "ServerMain.h"
#include "DataBase.h"

namespace SR {
	HANDLE g_iocp;
	std::array<CLIENT*, MAX_USER> g_users;
	std::priority_queue<EVENT> g_timer_queue;
	
	World* g_world;
	Map* g_map;

	std::default_random_engine g_dre{ 2016180012 };

	DataBase* g_db;
}

namespace ATOMIC {
	std::mutex g_timer_lock;
	std::mutex g_user_lock;
	std::mutex g_map_lock;
	std::mutex g_msg_lock;
}

int API_get_x(lua_State* L)
{
	int obj_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = SR::g_users[obj_id]->cur->GetPos().x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int obj_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = SR::g_users[obj_id]->cur->GetPos().y;
	lua_pushnumber(L, y);
	return 1;
}

int API_SendMessage(lua_State* L) {
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* message = (char*)lua_tostring(L, -1);

	ServerMain::GetInstance()->SendChatPacket(user_id, my_id, message, 0);

	lua_pop(L, 3);
	return 0;
}

void InitNPC() {
	std::uniform_int_distribution<int> uid_x{ 0, WORLD_WIDTH };
	std::uniform_int_distribution<int> uid_y{ 0, WORLD_HEIGHT };
		
	for (int i = NPC_ID_START; i < MAX_USER; ++i) {
		CLIENT* new_client = new CLIENT;
		new_client->user_id = i;
		new_client->m_recv_over.wsabuf[0].len = MAX_BUFFER;
		new_client->m_recv_over.wsabuf[0].buf = new_client->m_recv_over.net_buf;

		new_client->m_recv_over.ev_type = EV_RECV;
		new_client->isConnect = true;

		new_client->curr_packet_size = 0;
		new_client->prev_packet_data = 0;

		new_client->isAlive = true;
		new_client->isActive = false;

		new_client->cur = new Player;
		new_client->cur->SetHp(100);
		new_client->cur->SetLevel(1);
		new_client->cur->SetExp(0);
		new_client->cur->SetPos(POINT{ uid_x(SR::g_dre), uid_y(SR::g_dre) });
		new_client->cur->SetID(i);

		char npc_name[MAX_ID_LEN];
		sprintf_s(npc_name, "N%d", i);
		strcpy_s(new_client->id_str, npc_name);
		SR::g_users[i] = new_client;


		lua_State* L = SR::g_users[i]->L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "NPC.lua");
		lua_pcall(L, 0, 0, 0);
		
		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0);
		lua_pop(L, 1);

		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
	}
}

void CreatePlayerSlot(int maxslot) {
#ifdef LOG_ON
	cout << "CreatePlayerSlot start" << endl;
#endif
	for (int user_id = 0; user_id < maxslot; ++user_id) {
		CLIENT* player = new CLIENT;
		player->m_recv_over.wsabuf[0].len = MAX_BUFFER;
		player->m_recv_over.wsabuf[0].buf = player->m_recv_over.net_buf;
		player->user_id = user_id;
		ZeroMemory(player->packet_buf, MAX_BUFFER);
		ZeroMemory(player->id_str, sizeof(char) * MAX_ID_LEN);
		ATOMIC::g_user_lock.lock();
		SR::g_users[user_id] = player;
		ATOMIC::g_user_lock.unlock();
	}
#ifdef LOG_ON
	cout << "CreatePlayerSlot compleat" << endl;
#endif	
}

int main(int argc, char* argv[]) {
	std::wcout.imbue(std::locale("korean"));

	SR::g_db = new DataBase();
	SR::g_db->Init();

	CreatePlayerSlot(NPC_ID_START);

	ServerMain* server = ServerMain::GetInstance();
	//순서중요
	SR::g_map = new Map();
	SR::g_world = new World();
	InitNPC();

	std::cout << "Create player slot\n";

	server->Run();

	//for (int i = 0; i < WORLD_WIDTH; ++i) {
	//	for (int j = 0; j < WORLD_HEIGHT; ++j) {
	//		std::cout << static_cast<int>(SR::g_map->getData(i, j));
	//	}
	//	std::cout << std::endl;
	//}
}