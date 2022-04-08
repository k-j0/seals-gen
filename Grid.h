#pragma once

#include <vector>
#include <array>

#include "Vec.h"

/// 3D spatial data structure containing one vector of ints per cell, in NxNxN cells covering space between -0.5..0.5
class Grid {

private:

	/// Number of elements along one axis - grid size is resolution^3
	int resolution;

	std::vector<std::vector<int>> grid;

	/// Given a position in 3D space, returns the cell index
	inline int cellFromPosition(Vec3 pos) {
		int x = int((pos.X() + 0.5) * resolution);
		int y = int((pos.Y() + 0.5) * resolution);
		int z = int((pos.Z() + 0.5) * resolution);
		if (x < 0 || x >= resolution || y < 0 || y >= resolution || z < 0 || z >= resolution) return -1;
		return x + y * resolution + z * resolution * resolution;
	}

public:

	/// Constructor, given a cell side length; will create a grid with ceil(1/cellSize)^3 cells
	Grid (float cellSize);

	/// Clears all of the neighbour lists in the grid
	void clear ();

	/// Adds a value to the relevant grid cell
	void add (Vec3 pos, int value);

	/// Samples the grid, retaining the lists of neighbour cells
	/// Returns the cell contents for the 27 neighbouring cells:
	void sample (Vec3 pos, std::array<std::vector<int>*, 3*3*3>& neighbours);

};
