//lua �Լ� ȣ�� ����
#include <iostream>

extern "C" { //C�� ���ǵ� ���̺귯���ΰ��� ����(�����Ϸ����� �˷���)
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
};

#pragma comment(lib, "lua54.lib")
using namespace std;

int add_in_c(lua_State* L) {
	int a = lua_tonumber(L, -2);
	int b = lua_tonumber(L, -1);
	lua_pop(L, 3);
	int c = a + b;
	lua_pushnumber(L, c);
	return 1;
}

int main() {
	lua_State* L = luaL_newstate();		// ����ӽ� lua ����
	luaL_openlibs(L);					// �⺻ ���̺귯�� �ε�
	luaL_loadfile(L, "dragon.lua");		// ����ӽ�, ���α׷� �ڵ�, ���α׷� �ڵ� ����, ������(�ٴ���)
	int res = lua_pcall(L, 0, 0, 0);				// ����

	if (0 != res) {
		cout << "LUA error in exec: " << lua_tostring(L, -1) << endl;
		exit(-1);
	}
	//---------------------------------------------------------------------------

	//lua -> c++ ������ stack�� �׾Ƽ� ����
	lua_getglobal(L, "pos_x");
	lua_getglobal(L, "pos_y");
	
	//stack pointer�� 0 ��ǥ(�������)

	int dragon_x = lua_tonumber(L, -2); //stack pointer�� -2 ��ǥ(pos_x)
	int dragon_y = lua_tonumber(L, -1); //stack pointer�� -1 ��ǥ(pos_y)
	lua_pop(L, 2);

	cout << "dragon[" << dragon_x << ", "
		<< dragon_y << "]\n";

	//---------------------------------------------------------------------------

	lua_getglobal(L, "plustwo");	//����� �Լ�
	lua_pushnumber(L, 100);			//�Ķ����
	lua_pcall(L, 1, 1, 0);			//�Ķ���� ����, ���� ����(���ؿ� ��)
	int result = lua_tonumber(L, -1);
	lua_pop(L, 1);

	cout << "result: " << result << endl;
	//---------------------------------------------------------------------------

	lua_register(L, "call_c_func_add", add_in_c);
	lua_getglobal(L, "add_num");
	lua_pushnumber(L, 100);
	lua_pushnumber(L, 200);
	lua_pcall(L, 2, 1, 0);
	result = lua_tonumber(L, -1);
	lua_pop(L, 1);
	cout << "result: "<<result << endl;


	lua_close(L);						// ����ӽ� ����
}