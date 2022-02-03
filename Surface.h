#pragma once

#include <vector>
#include <random>
#include <ctime>
#include <string>
#include <unordered_set>
#include "Particle.h"

/// Represents a self-avoiding surface in 3D space, made up of a certain number of particles
class Surface {

public:

	/// Simulation parameters
	struct Params {

		double attractionMagnitude = .075;
		double repulsionMagnitudeFactor = 1.5; // * attractionMagnitude
		double damping = .15;
		double boundaryRadius = 1.;
		double boundaryExtent = .05; // 0..1
		double dt = .15;

	};

private:

	Params params;

	// Current timestep/iteration
	int t = 0;

	// Keep seed in memory
	int seed;

	// Random number generator to use for random operations
	std::mt19937 rng;

	// List of particles/vertices that make up the surface
	std::vector<Particle> particles;

	// List of triangles by particle/vertex indices
	std::vector<IVec3> triangles;

	// Edge map < vertex index -> [ nearest neighbour vertex indices ] >
	std::vector<std::unordered_set<int>> edges;

public:

	/// Default constructor
	Surface(Params params, int seed = time(nullptr));

	/// Adds a particle in a random location on the surface
	void addParticle ();

	/// Updates all particle accelerations/velocities/positions (advance one time step)
	/// Serial version
	void update ();

	/// Export to JSON, to be loaded into WebGL viewer
	std::string toJson();

};
