#pragma once

#include <vector>
#include <random>
#include <ctime>
#include <string>
#include "Particle.h"

/// Represents a self-avoiding surface in 3D space, made up of a certain number of particles
class Surface {

public:

	/// Simulation parameters
	struct Params {

		double attractionMagnitude = .075;

	};

private:

	Params params;

	// Keep seed in memory
	int seed;

	// Random number generator to use for random operations
	std::mt19937 rng;

	// List of particles/vertices that make up the surface
	std::vector<Particle> particles;

	// List of triangles by particle/vertex indices
	std::vector<IVec3> triangles;

public:

	/// Default constructor
	Surface(Params params, int seed = time(nullptr));

	/// Adds a particle in a random location on the surface
	void addParticle ();

	/// Export to JSON, to be loaded into WebGL viewer
	std::string toJson();

};
