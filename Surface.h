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


template<typename Bytes=bio::BufferedBinaryFileOutput<>>
class SurfaceBase {
public:
	virtual ~SurfaceBase() { }
	virtual void addParticle() = 0;
	virtual void update() = 0;
	virtual std::string toJson(int runtimeMs) = 0;
	virtual void toBinary(int runtimeMs, Bytes& data) = 0;
};


template<int D, typename neighbour_iterator_t, typename Bytes=bio::BufferedBinaryFileOutput<>>
class Surface : public SurfaceBase<Bytes> {

public:

	/// Simulation parameters
	struct Params {

		real_t attractionMagnitude = (real_t).025;
		real_t repulsionMagnitudeFactor = (real_t)2.1; // * attractionMagnitude
		real_t damping = (real_t).15;
		real_t pressure = (real_t)0;
		real_t targetVolume = real_t(-1); // can be left as -1 to compute from initial volume
		real_t noise = (real_t).25;
		Vec<real_t, D> repulsionAnisotropy = Vec<real_t, D>::One();
        real_t adaptiveRepulsion = real_t(0); // 0..1
        real_t rigidity = real_t(0); // 0..1
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
	
	// Must be implemented - returns the normal vector (pointing outwards) for a given particle
	virtual void computeNormals() {}
	virtual Vec<real_t, D> getNormal(int i) = 0;

	// Neighbour interfaces - must be implemented in derived classes
	virtual bool areNeighbours(int i, int j) = 0;
	virtual neighbour_iterator_t beginNeighbours(int i) = 0;
	virtual neighbour_iterator_t endNeighbours(int i) = 0;

	// Must be implemented in derived classes - returns a repulsion multiplier for two particles i and j
	virtual real_t getSurfaceTension(int i, int j) = 0;
	
	// Must be implemented in derived classes; returns the full volume/area of the surface
	virtual real_t getVolume() = 0;
	
	// Must be implemented in derived classes; returns a hint to indicate the type of surface this is, which will be inserted in output files
	virtual std::string getTypeHint() = 0;
    
    // Returns a constant factor corresponding to how much particle i should repulse non-neighbours
    real_t getRepulsion(int i) {
        if (params.adaptiveRepulsion <= real_t(0)) {
            return real_t(1.0);
        }
        
        // Find average distance to neighbours
        int neighbourCount = 0;
        real_t totalDistance = real_t(0.0);
        neighbour_iterator_t begin = beginNeighbours(i);
        neighbour_iterator_t end = endNeighbours(i);
        for (auto it = begin; it != end; it++) {
			Vec<real_t, D> towards = particles[*it].position - particles[i].position;
            totalDistance += std::sqrt(towards.lengthSqr());
            ++neighbourCount;
        }
        if (neighbourCount <= 0) return real_t(1.0);
        real_t avgDistance = totalDistance / neighbourCount;
        
        // particles that are n times as far from their neighbours as usual should repulse n times as much (lerp'ed to 1)
        return params.adaptiveRepulsion * avgDistance / params.attractionMagnitude + (real_t(1) - params.adaptiveRepulsion);
    }
    
    // Returns an estimate of the density locally around particle i
    // Counts the number of particles within circle of radius attraction magnitude
    int getNearbyParticleCount (int i) {
        int total = 0;
    #ifdef USE_GRID
		std::array<std::vector<int>*, powConstexpr(3, D)> cells;
		grid->sample(particles[i].position, cells);
		for (const std::vector<int>* const cell : cells) if (cell) for (const int& j : *cell) {
	#else // USE_GRID
		for (std::size_t j = 0; j < particles.size(); ++j) {
	#endif // !USE_GRID
			if (i == j) continue; // same particle
            Vec<real_t, D> towards = particles[j].position - particles[i].position;
            if (towards.lengthSqr() < params.attractionMagnitude * params.attractionMagnitude) {
                ++total;
            }
        }
        return total;
    }

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
	void toBinary(int runtimeMs, Bytes& data) final override;
	virtual void specificBinary(Bytes& data) = 0;

protected:

	inline real_t rand01() { return real_t(std::abs(int(rng())) % 10000) / (real_t)10000; }
	

	/// Should be called whenever a new particle is added
	inline void addParticleToGrid(int particle) {
		#ifdef USE_GRID
			particles[particle].position.clamp(real_t(-0.5), (real_t)0.4999);
			grid->add(particles[particle].position, particle);
		#endif // USE_GRID
	}

};



template<int D, typename neighbour_iterator_t, typename Bytes>
Surface<D, neighbour_iterator_t, Bytes>::Surface(Surface<D, neighbour_iterator_t, Bytes>::Params params, int seed) :
		params(params),
		seed(seed),
		rng(std::mt19937(seed)) {
	
	// create grid
#ifdef USE_GRID
	grid = std::make_unique<Grid<D>>(real_t(params.attractionMagnitude * std::max((real_t)1.0, params.repulsionMagnitudeFactor)));
#endif // USE_GRID
}


template<int D, typename neighbour_iterator_t, typename Bytes>
void Surface<D, neighbour_iterator_t, Bytes>::update() {

	int numParticles = (int)particles.size();
	
	// compute volume delta since beginning and resulting pressure force magnitude to apply to each particle
	bool boundaryNeedsVolume = !params.boundary ? false : params.boundary->needsVolume();
	real_t volume = params.pressure == 0 && !boundaryNeedsVolume ? 1 : // no need to compute volume without a pressure force or volume-based boundary growth
						std::max(real_t(0), getVolume());
	if (params.targetVolume < 0) params.targetVolume = volume;
	real_t pressureAmount = params.targetVolume == 0 ? 0 : params.pressure * (params.targetVolume - volume) / params.targetVolume; // increased volume: negative pressure; decreased volume: positive pressure
	if (pressureAmount != 0) {
		computeNormals();
	}

	// update acceleration values for all particles first without writing to position
	#pragma omp parallel for
	for (int i = 0; i < numParticles; ++i) {

		// particles attached to the wall should move towards their slot on the wall
		if (particles[i].attached) {
			assert(params.boundary);
			params.boundary->updateAttachedParticle(&particles[i], params.attractionMagnitude * std::max((real_t)1.0, params.repulsionMagnitudeFactor));
			continue;
		}
        
        // fully rigid particles should no longer move at all
        if (particles[i].flexibility <= 0.0) {
            continue;
        }

		// dampen acceleration
		particles[i].acceleration *= params.damping * params.damping;

		// boundary restriction force
		if (params.boundary) {
			particles[i].acceleration += params.boundary->force(particles[i].position);
		}
		
		// pressure force
		if (pressureAmount != 0) {
			Vec<real_t, D> normal = getNormal(i);
			normal *= pressureAmount;
			particles[i].acceleration += normal;
		}
		
		// iterate over non-neighbour particles
	#ifdef USE_GRID
		std::array<std::vector<int>*, powConstexpr(3, D)> cells;
		grid->sample(particles[i].position, cells);
		for (const std::vector<int>* const cell : cells) if (cell) for (const int& j : *cell) {
	#else // USE_GRID
		for (std::size_t j = 0; j < particles.size(); ++j) {
	#endif // !USE_GRID
			if (i == j || areNeighbours(i, j)) continue; // same particle, or nearest neighbours

			// repel if close enough
			Vec<real_t, D> towards = particles[j].position - particles[i].position;
			real_t noise = (1 + particles[i].noise * params.noise);
			real_t repulsionLen = params.attractionMagnitude * params.repulsionMagnitudeFactor * getSurfaceTension(i, j) * getRepulsion(j);
			real_t d2 = towards.lengthSqr() * noise * noise; // d^2 to skip sqrt most of the time
			if (d2 < repulsionLen * repulsionLen) {
				towards.normalize();
				towards *= std::sqrt(d2) - repulsionLen;
				particles[i].acceleration += towards.hadamard(params.repulsionAnisotropy);
			}
		}

		// iterate over neighbour particles
		neighbour_iterator_t neighboursBegin = beginNeighbours(i);
		neighbour_iterator_t neighboursEnd = endNeighbours(i);
		for (auto it = neighboursBegin; it != neighboursEnd; it++) {
			int neighbour = *it;

			// attract if far, repel if too close
			Vec<real_t, D> towards = particles[neighbour].position - particles[i].position;
			real_t d = std::sqrt(towards.lengthSqr());
			towards.normalize();
			towards *= d - params.attractionMagnitude;
			particles[i].acceleration += towards;
		}
	}

	// update positions for all particles
	#pragma omp parallel for
	for (int i = 0; i < numParticles; ++i) {

		// Ignore particles fixed in place
		if (particles[i].attached) continue;

		// dampen velocity
		particles[i].velocity *= params.damping;

		// apply acceleration
		particles[i].velocity += particles[i].acceleration * params.dt;

		// apply velocity
		particles[i].position += particles[i].velocity * params.dt * particles[i].flexibility;

		// apply hard boundary
		if (params.boundary) {
			params.boundary->hard(particles[i].position);
		}
        
        particles[i].flexibility *= (real_t(1.0) - params.rigidity);
        if (particles[i].flexibility < real_t(0)) {
            particles[i].flexibility = real_t(0);
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
		params.boundary->update(volume);
	}

	++t;
}


template<int D, typename neighbour_iterator_t, typename Bytes>
std::string Surface<D, neighbour_iterator_t, Bytes>::toJson(int runtimeMs) {

	std::string json = "{\n"
		"\t'date': " + std::to_string(time(nullptr)) + ",\n"
		"\t'machine': '" + getMachineName() + "',\n"
		"\t'seed': " + std::to_string(seed) + ",\n"
		"\t'dimension': " + std::to_string(D) + ",\n"
		"\t'hint': " + getTypeHint() + ",\n"
		"\t'timesteps': " + std::to_string(t) + ",\n"
		"\t'attractionMagnitude': " + std::to_string(params.attractionMagnitude) + ",\n"
		"\t'repulsionMagnitudeFactor': " + std::to_string(params.repulsionMagnitudeFactor) + ",\n"
		"\t'damping': " + std::to_string(params.damping) + ",\n"
		"\t'noise': " + std::to_string(params.noise) + ",\n"
		"\t'repulsionAnisotropy': " + params.repulsionAnisotropy.toString() + ",\n"
		"\t'boundary': " + (params.boundary ? params.boundary->toJson() : "null") + ",\n"
		"\t'dt': " + std::to_string(params.dt) + ",\n"
		"\t'runtime': " + std::to_string(runtimeMs) + ",\n"
		"\t'volume': " + std::to_string(getVolume()) + ",\n";

	specificJson(json);

	std::replace(json.begin(), json.end(), '\'', '"');
	return json + "}";
}

template<int D, typename neighbour_iterator_t, typename Bytes>
void Surface<D, neighbour_iterator_t, Bytes>::toBinary(int runtimeMs, Bytes& data) {

	// Header, in front of any surface object in the binary file
	data.push_back('S'); data.push_back('E'); data.push_back('L');
	
	// File version
	bio::writeSimple<std::uint8_t>(data, 4);
	
	// Metadata
	bio::writeSimple<std::uint8_t>(data, D);
	bio::writeString(data, getTypeHint());
	bio::writeSimple<int64_t>(data, time(nullptr));
	bio::writeString(data, getMachineName());
	bio::writeSimple<std::int32_t>(data, seed);
	bio::writeSimple<std::int32_t>(data, t);
	bio::writeSimple<real_t>(data, params.attractionMagnitude);
	bio::writeSimple<real_t>(data, params.repulsionMagnitudeFactor);
	bio::writeSimple<real_t>(data, params.damping);
	bio::writeSimple<real_t>(data, params.noise);
	bio::writeVec(data, params.repulsionAnisotropy);
	bio::writeSimple<real_t>(data, params.dt);
	bio::writeSimple<std::int32_t>(data, runtimeMs);
	bio::writeSimple<real_t>(data, getVolume());
	
	// Boundary
	if (params.boundary) {
		bio::writeSimple<std::int8_t>(data, 1);
		params.boundary->toBinary(data);
	} else {
		bio::writeSimple<std::int8_t>(data, 0);
	}

	// Core data
	bio::writeSimple<std::int32_t>(data, (std::int32_t)particles.size());
	specificBinary(data);

	// EOS
	data.push_back(0);
}

WARNING_POP;
