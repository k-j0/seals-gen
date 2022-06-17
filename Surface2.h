#pragma once

#include "Surface.h"


/// Represents a self-avoiding surface in 2D space, made up of a certain number of particles connected in one line
class Surface2 : public Surface<2> {

private:

	std::vector<int> nextNeighbourIndices;
	std::vector<int> previousNeighbourIndices;

public:

	Surface2(Params params, int seed = time(nullptr));

	void addParticle() override;

	void update() override;

	void specificJson(std::string& json) override;

	void specificBinary(std::vector<uint8_t>& data) override;

};
