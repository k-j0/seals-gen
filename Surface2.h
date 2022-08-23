#pragma once

#include "Surface.h"

#include "real.h"


/// Represents a self-avoiding surface in 2D space, made up of a certain number of particles connected in one line
class Surface2 : public Surface<2, std::array<int, 2>::const_iterator> {

public:

	struct SpecificParams {
		real_t channelRepulsionMultiplier = 1.0; // if != 1, repulsion across fingers will be different from repulsion across channels
	};

private:

	// Additional parameters on top of the Surface<2>::Params
	SpecificParams specificParams;

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

	inline real_t getRepulsion(int i, int j) override {
		if (specificParams.channelRepulsionMultiplier == 1.0) return 1.0;

		// Figure out whether the two particles are likely interfacing across a finger or a channel
		// Note: this is an approximation, and is likely to not work very well if particle i makes a very shallow or very large angle with its neighbours,
		// but in practice the angle is likely to be close to 180 deg and the interacting particle is likely to be close to 90 deg away
		// @todo: quantify false positives/negatives
		const std::array<int, 2>& iNeighbours = neighbourIndices[i];
		const Vec2& left = particles[iNeighbours[0]].position;
		const Vec2& right = particles[iNeighbours[1]].position;
		const Vec2 neighbourToNeighbour = (left - right).normalized();
		const Vec2 normal (neighbourToNeighbour.Y(), -neighbourToNeighbour.X());
		const Vec2 toJ = (particles[j].position - particles[i].position).normalized();
		if (normal.dot(toJ) < 0) {
			return specificParams.channelRepulsionMultiplier;
		} else {
			return 1.0;
		}
	}

public:

	Surface2(Params params, SpecificParams specificParams, int seed = time(nullptr));

	void addParticle() override;

	void specificJson(std::string& json) override;

	void specificBinary(std::vector<uint8_t>& data) override;

};
