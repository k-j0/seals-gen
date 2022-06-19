#pragma once

#include "Surface.h"


/// Represents a self-avoiding surface in 2D space, made up of a certain number of particles connected in one line
class Surface2 : public Surface<2, std::array<int, 2>::const_iterator> {

private:

	std::vector<std::array<int, 2>> neighbourIndices; // for each particle, neighbourIndices[i] provides the next ([1]) and previous ([0]) neighbour

protected:

	inline bool areNeighbours(int i, int j) override {
		return neighbourIndices[i][0] == j || neighbourIndices[j][0] == i;
	}

	inline std::array<int, 2>::const_iterator beginNeighbours(int i) override {
		return neighbourIndices[i].begin();
	}

	inline std::array<int, 2>::const_iterator endNeighbours(int i) override {
		return neighbourIndices[i].end();
	}

public:

	Surface2(Params params, int seed = time(nullptr));

	void addParticle() override;

	void specificJson(std::string& json) override;

	void specificBinary(std::vector<uint8_t>& data) override;

};
