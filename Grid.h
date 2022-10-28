#pragma once

#include <vector>
#include <array>
#include <cstring>

#include "Vec.h"
#include "Utils.h"



/// Spatial data structure containing one vector of ints per cell, in N^D cells covering space between -0.5..0.5
template<int D>
class Grid {

protected:

	/// Number of elements along one axis - grid size is resolution^D
	int resolution;

	std::vector<std::vector<int>> grid;

	/// Given a position in D space, returns the cell index
	inline int cellFromPosition(Vec<real_t, D> pos) const {
		pos += real_t(0.5);
		pos *= real_t(resolution);
		Vec<int, D> iPos = pos.floor();
		if (iPos < 0 || iPos >= resolution) return -1;
		return iPos.index(resolution);
	}

	/// Given a number from 0 to pow(3, D), returns its ternary representation
	inline void toTernary(int num, std::uint8_t digits[D]) {
		assert(num >= 0 && num < powConstexpr(3, D));
		
		std::memset(digits, 0, D * sizeof(std::uint8_t));
		
		int i = 0;
		do {
			int remainder = num % 3;
			num /= 3;
			assert(i < D);
			digits[i++] = remainder;
		} while (num >= 1);
	}

public:

	/// Constructor, given a cell side length; will create a grid with ceil(1/cellSize)^D cells
	Grid(real_t cellSize) {
		resolution = int(ceil(1.0 / cellSize));
		grid.resize(powConstexpr(resolution, D));
	}

	/// Clears all of the neighbour lists in the grid
	void clear() {
		for (auto& it : grid) {
			it.resize(0); // spec enforces this preserves the allocated capacity of the vector vs .clear()
		}
	}

	/// Adds a value to the relevant grid cell
	void add(Vec<real_t, D> pos, int value) {
		int idx = cellFromPosition(pos);
		assert(idx >= 0);
		assert((std::size_t)idx < grid.size());
		grid[idx].push_back(value);
	}

	/// Samples the grid, retaining the lists of neighbour cells
	/// Returns the cell contents for the 3^D neighbouring cells:
	void sample(Vec<real_t, D> pos, std::array<std::vector<int>*, powConstexpr(3, D)>& neighbours) {
		assert(pos >= -0.5 && pos < 0.5);

		// identity matrix divided by resolution
		real_t d = (real_t)1 / resolution;
		Vec<real_t, D> deltas[D];
		for (int i = 0; i < D; ++i) {
			deltas[i] = Vec<real_t, D>::Zero();
			deltas[i].set(i, d);
		}

		// Add all possible permutations of +/-/0 dx/dy/dz/...
		// i.e. pos + dx - dy, pos - dx, pos + dz, etc.
		std::uint8_t digits[D];
		for (int i = 0; i < powConstexpr(3, D); ++i) {

			// add offsets to position in the various axes
			Vec<real_t, D> p = pos;
			toTernary(i, digits);
			for (int j = 0; j < D; ++j) {
				assert(digits[j] < 3);
				if (digits[j] == 1) p -= deltas[j];
				else if (digits[j] == 2) p += deltas[j];
			}
			
			// grab index (might be invalid at boundaries, in which case pass nullptr in results)
			int idx = cellFromPosition(p);
			neighbours[i] = idx < 0 ? nullptr : &grid[idx];
		}
	}

};
