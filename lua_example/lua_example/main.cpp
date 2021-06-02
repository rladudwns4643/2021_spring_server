//lua 함수 호출 예시
#include <iostream>

extern "C" { //C로 정의된 라이브러리인것을 명시(컴파일러에게 알려줌)
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
};

#pragma comment(lib, "lua54.lib")
using namespace std;

int main() {
	lua_State* L = luaL_newstate();		// 가상머신 lua 생성
	luaL_openlibs(L);					// 기본 라이브러리 로드
	luaL_loadfile(L, "dragon.lua");		// 가상머신, 프로그램 코드, 프로그램 코드 길이, 실행방식(줄단위)
	int res = lua_pcall(L, 0, 0, 0);				// 실행

	if (0 != res) {
		cout << "LUA error in exec: " << lua_tostring(L, -1) << endl;
		exit(-1);
	}
	
	//lua -> c++ 변수를 stack에 쌓아서 전달
	lua_getglobal(L, "pos_x");
	lua_getglobal(L, "pos_y");
	
	//stack pointer의 0 좌표(비어있음)

	int dragon_x = lua_tonumber(L, -2); //stack pointer의 -2 좌표(pos_x)
	int dragon_y = lua_tonumber(L, -1); //stack pointer의 -1 좌표(pos_y)
	lua_pop(L, 2);

	cout << "dragon[" << dragon_x << ", "
		<< dragon_y << "]\n";

	lua_getglobal(L, "plustwo");	//사용할 함수
	lua_pushnumber(L, 100);			//파라미터
	lua_pcall(L, 1, 1, 0);			//파라미터 개수, 
	int result = lua_tonumber(L, -1);
	lua_pop(L, 1);

	cout << "result: " << result << endl;

	lua_close(L);						// 가상머신 종료
}