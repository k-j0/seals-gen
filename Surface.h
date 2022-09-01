#pragma once

#include <vector>
#include <random>
#include <ctime>
#include <string>
#include <unordered_set>
#include <memory>
#include <algorithm>

#include "Particle.h"
#include "SphereBoundary.h"
#include "Grid.h"
#include "Options.h"
#include "BinaryIO.h"
#include "Utils.h"
#include "warnings.h"


WARNING_PUSH;
WARNING_DISABLE_OMP_PRAGMAS;


class SurfaceBase {
public:
	virtual ~SurfaceBase() { }
	virtual void addParticle() = 0;
	virtual void update() = 0;
	virtual std::string toJson(int runtimeMs) = 0;
	virtual void toBinary(int runtimeMs, std::vector<uint8_t>& data) = 0;
};


template<int D, typename neighbour_iterator_t>
class Surface : public SurfaceBase {

public:

	/// Simulation parameters
	struct Params {

		real_t attractionMagnitude = (real_t).025;
		real_t repulsionMagnitudeFactor = (real_t)2.1; // * attractionMagnitude
		real_t damping = (real_t).15;
		real_t noise = (real_t).25;
		Vec<real_t, D> repulsionAnisotropy = Vec<real_t, D>::One();
		std::shared_ptr<BoundaryCondition<D>> boundary = nullptr;
		real_t dt = (real_t).15;

	};

protected:
	
	Params params;

	// Current timestep/iteration
	int t = 0;

	// Keep seed in memory
	int seed;

	// Random number generator to use for random operations
	std::mt19937 rng;

	// List of particles/vertices that make up the surface
	std::vector<Particle<D>> particles;

	// Grid - spatial acceleration data structure
	#ifdef USE_GRID
		std::unique_ptr<Grid<D>> grid;
	#endif // USE_GRID

	// Neighbour interfaces - must be implemented in derived classes
	virtual bool areNeighbours(int i, int j) = 0;
	virtual neighbour_iterator_t beginNeighbours(int i) = 0;
	virtual neighbour_iterator_t endNeighbours(int i) = 0;

	// Must be implemented in derived classes - returns a repulsion multiplier for two particles i and j
	virtual real_t getRepulsion(int i, int j) = 0;

public:
	
	/// Default constructor
	Surface(Params params, int seed);

	virtual ~Surface() override { }

	void update () override;

	/// Export to JSON, to be loaded into WebGL viewer
	std::string toJson(int runtimeMs) final override;
	virtual void specificJson(std::string& json) = 0;

	/// Export to minimal binary format
	/// Data may not be empty, in which case the surface info will be appended to the data vector, leaving existing contents as-is
	void toBinary(int runtimeMs, std::vector<uint8_t>& data) final override;
	virtual void specificBinary(std::vector<uint8_t>& data) = 0;

protected:

	inline real_t rand01() { return real_t(abs(int(rng())) % 10000) / (real_t)10000; }
	

	/// Should be called whenever a new particle is added
	inline void addParticleToGrid(int particle) {
		#ifdef USE_GRID
			particles[particle].position.clamp(real_t(-0.5), (real_t)0.4999);
			grid->add(particles[particle].position, particle);
		#endif // USE_GRID
	}

};



template<int D, typename neighbour_iterator_t>
Surface<D, neighbour_iterator_t>::Surface(Surface<D, neighbour_iterator_t>::Params params, int seed) :
		params(params),
		seed(seed),
		rng(std::mt19937(seed)) {
	
	// create grid
#ifdef USE_GRID
	grid = std::make_unique<Grid<D>>(real_t(params.attractionMagnitude * std::max((real_t)1.0, params.repulsionMagnitudeFactor)));
#endif // USE_GRID
}


template<int D, typename neighbour_iterator_t>
void Surface<D, neighbour_iterator_t>::update() {

	int numParticles = (int)particles.size();

	// update acceleration values for all particles first without writing to position
	#pragma omp parallel for
	for (int i = 0; i < numParticles; ++i) {

		// particles attached to the wall should move towards their slot on the wall
		if (particles[i].attached) {
			assert(params.boundary);
			params.boundary->updateAttachedParticle(&particles[i], params.attractionMagnitude * std::max((real_t)1.0, params.repulsionMagnitudeFactor));
			continue;
		}

		// dampen acceleration
		particles[i].acceleration.multiply(params.damping * params.damping);

		// boundary restriction force
		if (params.boundary) {
			particles[i].acceleration.add(params.boundary->force(particles[i].position));
		}

		// iterate over non-neighbour particles
	#ifdef USE_GRID
		std::array<std::vector<int>*, powConstexpr(3, D)> cells;
		grid->sample(particles[i].position, cells);
		for (const std::vector<int>* const cell : cells) if (cell) for (const int& j : *cell) {
	#else // USE_GRID
		for (size_t j = 0; j < particles.size(); ++j) {
	#endif // !USE_GRID
			if (i == j || areNeighbours(i, j)) continue; // same particle, or nearest neighbours

			// repel if close enough
			Vec<real_t, D> towards = particles[j].position - particles[i].position;
			real_t noise = (1 + particles[i].noise * params.noise);
			real_t repulsionLen = params.attractionMagnitude * params.repulsionMagnitudeFactor * getRepulsion(i, j);
			real_t d2 = towards.lengthSqr() * noise * noise; // d^2 to skip sqrt most of the time
			if (d2 < repulsionLen * repulsionLen) {
				towards.normalize();
				towards.multiply(sqrt(d2) - repulsionLen);
				particles[i].acceleration.add(towards.hadamard(params.repulsionAnisotropy));
			}
		}

		// iterate over neighbour particles
		neighbour_iterator_t neighboursBegin = beginNeighbours(i);
		neighbour_iterator_t neighboursEnd = endNeighbours(i);
		for (auto it = neighboursBegin; it != neighboursEnd; it++) {
			int neighbour = *it;

			// attract if far, repel if too close
			Vec<real_t, D> towards = particles[neighbour].position - particles[i].position;
			real_t d = sqrt(towards.lengthSqr());
			towards.normalize();
			towards.multiply(d - params.attractionMagnitude);
			particles[i].acceleration.add(towards);
		}
	}

	// update positions for all particles
	#pragma omp parallel for
	for (int i = 0; i < numParticles; ++i) {

		// Ignore particles fixed in place
		if (particles[i].attached) continue;

		// dampen velocity
		particles[i].velocity.multiply(params.damping);

		// apply acceleration
		particles[i].velocity.add(particles[i].acceleration * params.dt);

		// apply velocity
		particles[i].position.add(particles[i].velocity * params.dt);

		// apply hard boundary
		if (params.boundary) {
			params.boundary->hard(particles[i].position);
		}
	}

	// Update grid
	#ifdef USE_GRID
		grid->clear();
		for (int i = 0; i < numParticles; ++i) {
			addParticleToGrid(i);
		}
	#endif

	// Update boundary condition
	if (params.boundary) {
		params.boundary->update();
	}

	++t;
}


template<int D, typename neighbour_iterator_t>
std::string Surface<D, neighbour_iterator_t>::toJson(int runtimeMs) {

	std::string json = "{\n"
		"\t'date': " + std::to_string(time(nullptr)) + ",\n"
		"\t'machine': '" + getMachineName() + "',\n"
		"\t'seed': " + std::to_string(seed) + ",\n"
		"\t'dimension': " + std::to_string(D) + ",\n"
		"\t'timesteps': " + std::to_string(t) + ",\n"
		"\t'attractionMagnitude': " + std::to_string(params.attractionMagnitude) + ",\n"
		"\t'repulsionMagnitudeFactor': " + std::to_string(params.repulsionMagnitudeFactor) + ",\n"
		"\t'damping': " + std::to_string(params.damping) + ",\n"
		"\t'noise': " + std::to_string(params.noise) + ",\n"
		"\t'repulsionAnisotropy': " + params.repulsionAnisotropy.toString() + ",\n"
		"\t'boundary': " + (params.boundary ? params.boundary->toJson() : "null") + ",\n"
		"\t'dt': " + std::to_string(params.dt) + ",\n"
		"\t'runtime': " + std::to_string(runtimeMs) + ",\n";

	specificJson(json);

	std::replace(json.begin(), json.end(), '\'', '"');
	return json + "}";
}

template<int D, typename neighbour_iterator_t>
void Surface<D, neighbour_iterator_t>::toBinary(int runtimeMs, std::vector<uint8_t>& data) {

	// Header, in front of any surface object in the binary file
	data.push_back('S'); data.push_back('R'); data.push_back('F');

	// Metadata
	bio::writeSimple<uint8_t>(data, D);
	bio::writeSimple<int64_t>(data, time(nullptr));
	bio::writeString(data, getMachineName());
	bio::writeSimple<int32_t>(data, seed);
	bio::writeSimple<int32_t>(data, t);
	bio::writeSimple<real_t>(data, params.attractionMagnitude);
	bio::writeSimple<real_t>(data, params.repulsionMagnitudeFactor);
	bio::writeSimple<real_t>(data, params.damping);
	bio::writeSimple<real_t>(data, params.noise);
	bio::writeVec(data, params.repulsionAnisotropy);
	bio::writeSimple<real_t>(data, params.dt);
	bio::writeSimple<int32_t>(data, runtimeMs);

	// Core data
	bio::writeSimple<int32_t>(data, (int32_t)particles.size());
	specificBinary(data);

	// EOS
	data.push_back(0);
}

WARNING_POP;
