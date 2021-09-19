#pragma once
#include "pch.h"
#include "TCPSocket.h"

typedef short SHORT;

namespace GLOBAL_DEFINE {
	constexpr SHORT MAIN_SERVER_PORT = 9000;
	constexpr UINT EVENT_KEY = 100;
	constexpr SHORT NO_MSG = -1;
}

namespace REVIVAL_POINT {
	constexpr POINT pt[10] = {
	{123,012},
	{234,901},
	{345,890},
	{456,789},
	{567,678},
	{678,567},
	{789,456},
	{890,345},
	{901,234},
	{012,123}
	};
}

namespace NETWORK_UTIL {
	constexpr int WORKER_THREAD_NUM = 8;
}

enum EVENT_TYPE {
	EV_RECV,
	EV_SEND,
	EV_UPDATE,
	EV_FLUSH_MSG,
	EV_RANDOM_MOVE,
	EV_PLAYER_MOVE_NOTIFY,
	EV_TYPE_COUNT
};

struct EX_OVER {
	WSAOVERLAPPED	over;
	WSABUF			wsabuf[1];
	char			net_buf[MAX_BUFFER];
	EVENT_TYPE		ev_type;
	int				obj_id; //only EV_PLAYER_MOVE_NOTIFY
};

class Player;
struct CLIENT {
	EX_OVER					m_recv_over;
	TCPSocketPtr			m_s;
	UCHAR					packet_buf[MAX_BUFFER];
	int						prev_packet_data;
	int						curr_packet_size;
	UINT					user_id;

	bool					isConnect = false;
	std::atomic_bool		isActive = false;
	bool					isAlive = false;
	char					id_str[MAX_ID_LEN];

	std::unordered_set<int>	viewlist;
	std::mutex				viewlist_lock;

	lua_State*				L;
	std::mutex				lua_lock;

	Player*					cur = nullptr; //world에서 할당

	UINT					move_time;
};

struct EVENT {
	int id;
	std::chrono::high_resolution_clock::time_point event_time;
	EVENT_TYPE et;
	int tmp;
	constexpr bool operator<(const EVENT& rhs) const { return event_time > rhs.event_time; }
};

struct message {
	UINT id;
	char msg_type;
	POINT pos;
	char chatbuf[MAX_CHAT_LEN];
	int dir;
};

struct SEND_INFO {
	UINT to_id;
	char buf[MAX_BUFFER];
};

struct Node {
	POINT curLocation;
	POINT prevLocation;
};

struct Huristic {
	int movingDist;
	float preDist;
};