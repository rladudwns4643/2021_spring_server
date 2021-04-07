#pragma once

constexpr int MAX_BUFFER = 1024;
constexpr short SERVER_PORT = 3500;
constexpr int MAX_USER = 10;

#define SERVER_LISTEN_KEY	0 //server_id

constexpr unsigned int MAX_NAME_LEN = 100;

constexpr unsigned char CS_LOGIN = 1;
constexpr unsigned char CS_MOVE = 2;
constexpr unsigned char SC_LOGIN_OKAY = 3;
constexpr unsigned char SC_ADD_PLAYER = 4;
constexpr unsigned char SC_MOVE_PLAYER = 5;
constexpr unsigned char SC_REMOVE_PLAYER = 6;

constexpr int WORLD_X_SIZE = 8;
constexpr int WORLD_Y_SIZE = 8;

typedef unsigned char BYTE;

#pragma pack(push,1)
struct c2s_packet_login{
	BYTE size;
	BYTE type;
	char name[MAX_NAME_LEN];
};

enum DIRECTION { D_L, D_U, D_R, D_D, D_NULL};
struct c2s_packet_move {
	BYTE size;
	BYTE type;
	DIRECTION dir; //0: left, 1: up, 2: right, 3: down
};

struct s2c_packet_login_okay {
	BYTE size;
	BYTE type;
	int id;
	short x, y;
	int hp, level;
	int race;
};

struct s2c_packet_add_player{
	BYTE size;
	BYTE type;
	int id;
	short x, y;
	int race;
};

struct s2c_packet_move_player {
	BYTE size;
	BYTE type;
	int id;
	short x, y;
};

struct s2c_packet_remove_player {
	BYTE size;
	BYTE type;
	int id;
};
#pragma pack(pop)