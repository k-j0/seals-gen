#pragma once

#include "Surface.h"


/// Represents a self-avoiding surface in 3D space, made up of a certain number of particles
class Surface3 : public Surface<3, std::unordered_set<int>::const_iterator> {

public:

	enum class GrowthStrategy {
		ON_EDGE, // selects a random edge and places new particle in its centre
		DELAUNAY, // places a random particle on the unit sphere and connect it to neighbours in spherical coords
		DELAUNAY_ANISO_EDGE // places a random particle on the unit sphere between two edges with anisotropy
	};

	struct SpecificParams {
		GrowthStrategy strategy = GrowthStrategy::DELAUNAY;
	};

private:

	// Additional parameters on top of the Surface<3>::Params
	SpecificParams specificParams;

	// List of triangles by particle/vertex indices
	std::vector<IVec3> triangles;

	// Edge map < vertex index -> [ nearest neighbour vertex indices ] >
	std::vector<std::unordered_set<int>> edges;

protected:

	inline bool areNeighbours(int i, int j) override {
		return edges[i].find(j) != edges[i].end();
	}

	inline std::unordered_set<int>::const_iterator beginNeighbours(int i) override {
		return edges[i].begin();
	}

	inline std::unordered_set<int>::const_iterator endNeighbours(int i) override {
		return edges[i].end();
	}

public:
	
	Surface3(Params params, SpecificParams specificParams, int seed = time(nullptr));

	/// Adds a particle in a random location on the surface
	void addParticle() override;

	/// Add specific info to the json string
	void specificJson(std::string& json) override;

	/// Add specific info to the binary stream
	void specificBinary(std::vector<uint8_t>& data) override;

private:

	/// Adds a particle between any two existing particles and connects to neighbouring triangles
	void addParticleEdge();

	/// Adds a particle in a random location on the surface, using Delaunay triangulation
	void addParticleDelaunay();

	/// Adds a particle on an aligned edge (anisotropic growth), using Delaunay trigulation
	void addParticleEdgeDelaunay();

};
