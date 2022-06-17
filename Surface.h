#pragma once

#include <vector>
#include <random>
#include <ctime>
#include <string>
#include <unordered_set>
#include <memory>

#include "Particle.h"
#include "SphereBoundary.h"
#include "Grid.h"
#include "Options.h"
#include "BinaryIO.h"
#include "Utils.h"


class SurfaceBase {
public:
	virtual void addParticle() = 0;
	virtual void update() = 0;
	virtual std::string toJson(int runtimeMs) = 0;
	virtual void toBinary(int runtimeMs, std::vector<uint8_t>& data) = 0;
};


template<int D>
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

public:
	
	/// Default constructor
	Surface(Params params, int seed);

	/// Export to JSON, to be loaded into WebGL viewer
	std::string toJson(int runtimeMs) final override;
	virtual void specificJson(std::string& json) = 0;

	/// Export to minimal binary format
	/// Data may not be empty, in which case the surface info will be appended to the data vector, leaving existing contents as-is
	void toBinary(int runtimeMs, std::vector<uint8_t>& data) final override;
	virtual void specificBinary(std::vector<uint8_t>& data) = 0;

protected:

	inline double rand01() { return double(abs(int(rng())) % 10000) / 10000.0; }

};



template<int D>
Surface<D>::Surface(Surface<D>::Params params, int seed) :
	params(params),
	seed(seed),
	rng(std::mt19937(seed)) { }


template<int D>
std::string Surface<D>::toJson(int runtimeMs) {

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

template<int D>
void Surface<D>::toBinary(int runtimeMs, std::vector<uint8_t>& data) {

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

