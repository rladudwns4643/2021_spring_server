#include <iostream>

extern "C" { //C로 정의된 라이브러리인것을 명시(컴파일러에게 알려줌)
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
}

#pragma comment(lib, "lua54.lib");
using namespace std;

int main() {
	const char* lua_pro = "print \"HELL WORLD from LUA\"";

	lua_State* L = luaL_newstate();		// 가상머신 lua 생성
	luaL_openlibs(L);					// 기본 라이브러리 로드
	luaL_loadbuffer(L, lua_pro, strlen(lua_pro), "line");		// 가상머신, 프로그램 코드, 프로그램 코드 길이, 실행방식(줄단위)
	lua_pcall(L, 0, 0, 0);				// 실행
	lua_close(L);						// 가상머신 종료
}