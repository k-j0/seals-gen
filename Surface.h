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


/// Represents a self-avoiding surface in 3D space, made up of a certain number of particles
class Surface {

public:

	/// Simulation parameters
	struct Params {

		double attractionMagnitude = .025;
		double repulsionMagnitudeFactor = 2.1; // * attractionMagnitude
		double damping = .15;
		double noise = .25;
		Vec3 repulsionAnisotropy = Vec3::One();
		std::shared_ptr<BoundaryCondition> boundary = std::shared_ptr<BoundaryCondition>(new SphereBoundary);
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

	// Grid - spatial acceleration data structure
#ifdef USE_GRID
	std::unique_ptr<Grid> grid;
#endif // USE_GRID

public:

	/// Default constructor
	Surface(Params params, int seed = time(nullptr));

	/// Adds a particle in a random location on the surface
	void addParticle ();

	/// Adds a particle in a random location on the surface, using Delaunay triangulation
	void addParticleDelaunay ();

	/// Adds a particle on an aligned edge (anisotropic growth), using Delaunay trigulation
	void addParticleEdgeDelaunay ();

	/// Updates all particle accelerations/velocities/positions (advance one time step)
	/// Serial version
	void update ();

	/// Export to JSON, to be loaded into WebGL viewer
	std::string toJson();

private:

	/// Called whenever a new particle is added
#ifdef USE_GRID
	inline void addParticleToGrid(int particle) {
		if (particles[particle].position.X() < -0.5) particles[particle].position.setX(-0.5);
		if (particles[particle].position.X() >= 0.5) particles[particle].position.setX(0.4999);
		if (particles[particle].position.Y() < -0.5) particles[particle].position.setY(-0.5);
		if (particles[particle].position.Y() >= 0.5) particles[particle].position.setY(0.4999);
		if (particles[particle].position.Z() < -0.5) particles[particle].position.setZ(-0.5);
		if (particles[particle].position.Z() >= 0.5) particles[particle].position.setZ(0.4999);
		grid->add(particles[particle].position, particle);
	}
#else // USE_GRID
	inline void addParticleToGrid(int particle) { }
#endif // !USE_GRID

};
