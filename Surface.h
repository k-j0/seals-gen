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


#pragma warning(push)
#pragma warning(disable: 6993) // Ignore MSVC complaining about omp pragmas


class SurfaceBase {
public:
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

		double attractionMagnitude = .025;
		double repulsionMagnitudeFactor = 2.1; // * attractionMagnitude
		double damping = .15;
		double noise = .25;
		Vec<double, D> repulsionAnisotropy = Vec<double, D>::One();
		std::shared_ptr<BoundaryCondition<D>> boundary = nullptr;
		double dt = .15;

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

public:
	
	/// Default constructor
	Surface(Params params, int seed);

	virtual ~Surface() { }

	void update () override;

	/// Export to JSON, to be loaded into WebGL viewer
	std::string toJson(int runtimeMs) final override;
	virtual void specificJson(std::string& json) = 0;

	/// Export to minimal binary format
	/// Data may not be empty, in which case the surface info will be appended to the data vector, leaving existing contents as-is
	void toBinary(int runtimeMs, std::vector<uint8_t>& data) final override;
	virtual void specificBinary(std::vector<uint8_t>& data) = 0;

protected:

	inline double rand01() { return double(abs(int(rng())) % 10000) / 10000.0; }
	

	/// Should be called whenever a new particle is added
	inline void addParticleToGrid(int particle) {
		#ifdef USE_GRID
			particles[particle].position.clamp(-0.5, 0.4999);
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
	grid = std::make_unique<Grid<D>>(float(params.attractionMagnitude * std::max(1.0, params.repulsionMagnitudeFactor)));
#endif // USE_GRID
}


template<int D, typename neighbour_iterator_t>
void Surface<D, neighbour_iterator_t>::update() {
	
	int numParticles = (int)particles.size();

	// update acceleration values for all particles first without writing to position
	#pragma omp parallel for
	for (int i = 0; i < numParticles; ++i) {

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
			Vec<double, D> towards = particles[j].position - particles[i].position;
			double noise = (1 + particles[i].noise * params.noise);
			double repulsionLen = params.attractionMagnitude * params.repulsionMagnitudeFactor;
			double d2 = towards.lengthSqr() * noise * noise; // d^2 to skip sqrt most of the time
			if (d2 < repulsionLen * repulsionLen) {
				towards.normalize();
				towards.multiply(sqrt(d2) - repulsionLen);
				particles[i].acceleration.add(towards <hadamard> params.repulsionAnisotropy);
			}
		}

		// iterate over neighbour particles
		neighbour_iterator_t neighboursBegin = beginNeighbours(i);
		neighbour_iterator_t neighboursEnd = endNeighbours(i);
		for (auto it = neighboursBegin; it != neighboursEnd; it++) {
			int neighbour = *it;

			// attract if far, repel if too close
			Vec<double, D> towards = particles[neighbour].position - particles[i].position;
			double d = sqrt(towards.lengthSqr());
			towards.normalize();
			towards.multiply(d - params.attractionMagnitude);
			particles[i].acceleration.add(towards);
		}
	}

	// update positions for all particles
	#pragma omp parallel for
	for (int i = 0; i < numParticles; ++i) {

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
	bio::writeSimple<long long>(data, time(nullptr));
	bio::writeString(data, getMachineName());
	bio::writeSimple<int>(data, seed);
	bio::writeSimple<int>(data, t);
	bio::writeSimple<double>(data, params.attractionMagnitude);
	bio::writeSimple<double>(data, params.repulsionMagnitudeFactor);
	bio::writeSimple<double>(data, params.damping);
	bio::writeSimple<double>(data, params.noise);
	bio::writeVec(data, params.repulsionAnisotropy);
	bio::writeSimple<double>(data, params.dt);
	bio::writeSimple<int>(data, runtimeMs);

	// Core data
	specificBinary(data);

	// EOS
	data.push_back(0);
}

#pragma warning(pop)
