#pragma once

#include "Surface.h"

#include "real.h"


/// Represents a self-avoiding surface in 2D space, made up of a certain number of particles connected in one line
class Surface2 : public Surface<2, std::array<int, 2>::const_iterator> {

public:

	struct SpecificParams {
		int initialParticleCount = 3;
		real_t initialNoise = 0;
		bool attachFirstParticle = false; // if true, attaches the first particle to the boundary wall
		real_t surfaceTensionMultiplier = 1.0; // if > 1, repulsion for next-neighbours will be higher than higher-order neighbours
	};

private:

	// Additional parameters on top of the Surface<2>::Params
	SpecificParams specificParams;

	std::vector<std::array<int, 2>> neighbourIndices; // for each particle, neighbourIndices[i] provides the next ([1]) and previous ([0]) neighbour

protected:
	
	inline Vec2 getNormal(int i) override {
		Vec2 toCurr = particles[i].position - particles[neighbourIndices[i][0]].position;
		Vec2 toNext = particles[neighbourIndices[i][1]].position - particles[i].position;
		Vec2 normal = { toCurr.Y(), -toCurr.X() }; // normal of vector from previous to current
		normal += { toNext.Y(), -toNext.X() }; // + normal of vector from current to next
		normal.normalize(); // normalize (i.e. average then normalize)
		return normal;
	}
	
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
		if (specificParams.surfaceTensionMultiplier == 1.0) return 1.0;

		// If the particles are next-nearest neighbours, apply multiplier
		const std::array<int, 2>& directNeighbours = neighbourIndices[i];
		const std::array<int, 2>& nextNeighboursLeft = neighbourIndices[directNeighbours[0]];
		const std::array<int, 2>& nextNeighboursRight = neighbourIndices[directNeighbours[1]];
		if (nextNeighboursLeft[0] == j || nextNeighboursLeft[1] == j || nextNeighboursRight[0] == j || nextNeighboursRight[1] == j) {
			return specificParams.surfaceTensionMultiplier;
		} else {
			return 1.0;
		}
	}
	
	inline real_t getVolume() override {
		// compute area enclosed within the polygon
		real_t area = 0;
		#pragma omp parallel for reduction(+: area)
		for (std::size_t i = 0; i < particles.size(); ++i) {
			const std::array<int, 2>& neighbours = neighbourIndices[i];
			area += particles[i].position.X() * (particles[neighbours[1]].position.Y() - particles[neighbours[0]].position.Y());
		}
		return area * 0.5;
	}

public:

	Surface2(Params params, SpecificParams specificParams, int seed = time(nullptr));

	void addParticle() override;

	void specificJson(std::string& json) override;

	void specificBinary(std::vector<uint8_t>& data) override;

};
