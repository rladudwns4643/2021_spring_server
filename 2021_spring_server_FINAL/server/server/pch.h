#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma comment(lib, "ws2_32")
#include <WinSock2.h>

#include <functional>
#include <assert.h>

#include <thread>
#include <mutex>
#include <shared_mutex>
#include <chrono>

#include <array>
#include <vector>
#include <list>
#include <string>
#include <queue>
#include <random>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <typeinfo>
#include <map>

#include <sqlext.h>

extern "C" {
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
}
#pragma comment(lib, "lua54.lib")

#include "2021_server_protocol.h"