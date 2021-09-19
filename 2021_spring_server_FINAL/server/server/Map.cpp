#include "pch.h"
#include "extern.h"
#include "Map.h"

Map::Map() {
	MakeNewMap();
}

Map::Map(const Map& rhs)
{
	if (this != &rhs) {
		for (int i = 0; i < WORLD_HEIGHT; ++i) {
			for (int j = 0; j < WORLD_WIDTH; ++j) {
				data[i][j] = rhs.data[i][j];
			}
		}
	}
}

Map::~Map() {

}

void Map::MakeNewMap() {
	std::uniform_int_distribution<int> uid{ 0, 4 };
	for (int i = 0; i < WORLD_HEIGHT; ++i) {
		for (int j = 0; j < WORLD_WIDTH; ++j) {
			if (uid(SR::g_dre) == 0) {
				data[i][j] = GRID_TYPE::eBLOCKED;
			}
			else {
				data[i][j] = GRID_TYPE::eBLANK;
			}
		}
	}
	std::cout << "complete Make Map\n";
}

int Map::getData(int x, int y)
{
	return data[x][y];
}

void Map::setData(int x, int y, int d) {
	data[x][y] = d;
}