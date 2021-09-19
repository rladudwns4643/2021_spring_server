#pragma once
#pragma once
#include "server_define.h"

class Map {
public:
	Map();
	Map(const Map& rhs);
	~Map();

	void MakeNewMap();

	int getData(int x, int y);
	void setData(int x, int y, int d);

	bool MakeMapComplite = false;

public:
	int data[WORLD_WIDTH][WORLD_HEIGHT];
};