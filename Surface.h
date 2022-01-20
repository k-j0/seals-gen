#pragma once

#include <vector>
#include <random>
#include <ctime>
#include <string>
#include "Particle.h"

/// Represents a self-avoiding surface in 3D space, made up of a certain number of particles
class Surface {

	// Keep seed in memory
	int seed;

	// Random number generator to use for random operations
	std::mt19937 rng;

	// List of particles that make up the surface
	std::vector<Particle> particles;

public:

	/// Default constructor
	Surface(int initialParticleCount, int seed = time(nullptr));

	/// Adds a particle in a random location on the surface
	void addParticle ();

	/// Export to JSON, to be loaded into WebGL viewer
	std::string toJson();

};
