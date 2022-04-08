
#include "Grid.h"

#define CHECK_POS_VERBOSE(pos) \
		if (pos.X() < -0.5) { printf("Pos.X < -0.5 (%f)\n", pos.X()); assert(false); } \
		if (pos.X() >= 0.5) { printf("Pos.X >= 0.5 (%f)\n", pos.X()); assert(false); } \
		if (pos.Y() < -0.5) { printf("Pos.Y < -0.5 (%f)\n", pos.Y()); assert(false); } \
		if (pos.Y() >= 0.5) { printf("Pos.Y >= 0.5 (%f)\n", pos.Y()); assert(false); } \
		if (pos.Z() < -0.5) { printf("Pos.Z < -0.5 (%f)\n", pos.Z()); assert(false); } \
		if (pos.Z() >= 0.5) { printf("Pos.Z >= 0.5 (%f)\n", pos.Z()); assert(false); }

#define CHECK_POS_ASSERT(pos) \
		assert(pos.X() >= -0.5); assert(pos.X() < 0.5); \
		assert(pos.Y() >= -0.5); assert(pos.Y() < 0.5); \
		assert(pos.Z() >= -0.5); assert(pos.Z() < 0.5);

#define CHECK_POS CHECK_POS_ASSERT

Grid::Grid(float cellSize) {
	
	resolution = int(ceil(1.0 / cellSize));
	grid.resize(resolution * resolution * resolution);

}

void Grid::clear() {
	for (auto& it : grid) {
		it.clear();
	}
}

void Grid::add(Vec3 pos, int value) {
	CHECK_POS(pos);
	int idx = cellFromPosition(pos);
	assert(idx >= 0);
	grid[idx].push_back(value);
}

void Grid::sample(Vec3 pos, std::array<std::vector<int>*, 3*3*3>& neighbours) {

	CHECK_POS(pos);

	double d = 1.0 / resolution;
	Vec3 dx(d, 0.0, 0.0);
	Vec3 dy(0.0, d, 0.0);
	Vec3 dz(0.0, 0.0, d);

#define CELL(idx) (idx < 0 ? nullptr : &grid[idx])

	neighbours[ 0] = CELL(cellFromPosition(pos));
	neighbours[ 1] = CELL(cellFromPosition(pos+dx));
	neighbours[ 2] = CELL(cellFromPosition(pos-dx));
	neighbours[ 3] = CELL(cellFromPosition(pos+dy));
	neighbours[ 4] = CELL(cellFromPosition(pos-dy));
	neighbours[ 5] = CELL(cellFromPosition(pos+dz));
	neighbours[ 6] = CELL(cellFromPosition(pos-dz));
	neighbours[ 7] = CELL(cellFromPosition(pos+dx+dy));
	neighbours[ 8] = CELL(cellFromPosition(pos+dx-dy));
	neighbours[ 9] = CELL(cellFromPosition(pos+dx+dz));
	neighbours[10] = CELL(cellFromPosition(pos+dx-dz));
	neighbours[11] = CELL(cellFromPosition(pos-dx+dy));
	neighbours[12] = CELL(cellFromPosition(pos-dx-dy));
	neighbours[13] = CELL(cellFromPosition(pos-dx+dz));
	neighbours[14] = CELL(cellFromPosition(pos-dx-dz));
	neighbours[15] = CELL(cellFromPosition(pos+dy+dz));
	neighbours[16] = CELL(cellFromPosition(pos+dy-dz));
	neighbours[17] = CELL(cellFromPosition(pos-dy+dz));
	neighbours[18] = CELL(cellFromPosition(pos-dy-dz));
	neighbours[19] = CELL(cellFromPosition(pos+dx+dy+dz));
	neighbours[20] = CELL(cellFromPosition(pos+dx+dy-dz));
	neighbours[21] = CELL(cellFromPosition(pos+dx-dy+dz));
	neighbours[22] = CELL(cellFromPosition(pos+dx-dy-dz));
	neighbours[23] = CELL(cellFromPosition(pos-dx+dy+dz));
	neighbours[24] = CELL(cellFromPosition(pos-dx+dy-dz));
	neighbours[25] = CELL(cellFromPosition(pos-dx-dy+dz));
	neighbours[26] = CELL(cellFromPosition(pos-dx-dy-dz));

#undef CELL

}
