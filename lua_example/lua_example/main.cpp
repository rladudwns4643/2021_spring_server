#include <iostream>

extern "C" { //C�� ���ǵ� ���̺귯���ΰ��� ���(�����Ϸ����� �˷���)
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
}

#pragma comment(lib, "lua54.lib");
using namespace std;

int main() {
	const char* lua_pro = "print \"HELL WORLD from LUA\"";

	lua_State* L = luaL_newstate();		// ����ӽ� lua ����
	luaL_openlibs(L);					// �⺻ ���̺귯�� �ε�
	luaL_loadbuffer(L, lua_pro, strlen(lua_pro), "line");		// ����ӽ�, ���α׷� �ڵ�, ���α׷� �ڵ� ����, ������(�ٴ���)
	lua_pcall(L, 0, 0, 0);				// ����
	lua_close(L);						// ����ӽ� ����
}