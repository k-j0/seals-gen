#pragma once

#include "Surface.h"

#include "real.h"


/// Represents a self-avoiding surface in 3D space, made up of a certain number of particles
class Surface3 : public Surface<3, std::unordered_set<int>::const_iterator> {

public:

	enum class GrowthStrategy {
		ON_EDGE, // selects a random edge and places new particle in its centre
		DELAUNAY, // places a random particle on the unit sphere and connect it to neighbours in spherical coords
		DELAUNAY_ANISO_EDGE // places a random particle on the unit sphere between two edges with anisotropy
	};

	struct SpecificParams {
		bool attachFirstParticle = false; // if true, attaches the first particle to the boundary wall (@todo: would be better to attach a group of particles e.g. in a row)
		GrowthStrategy strategy = GrowthStrategy::DELAUNAY;
		real_t surfaceTensionMultiplier = real_t(1.0);
	};

private:

	// Additional parameters on top of the Surface<3>::Params
	SpecificParams specificParams;

	// List of triangles
	std::vector<IVec3> triangles;
	
	// List of vertex normals (same length as particles)
	std::vector<Vec3> normals;

	// Edge map < vertex index -> [ nearest neighbour vertex indices ] >
	std::vector<std::unordered_set<int>> edges;

protected:
	
	void computeNormals () override;
	
	inline Vec3 getNormal(int i) override {
		if (i >= (int)normals.size()) return Vec3::Zero();
		return normals[i];
	}
	
	inline bool areNeighbours(int i, int j) override {
		return edges[i].find(j) != edges[i].end();
	}

	inline std::unordered_set<int>::const_iterator beginNeighbours(int i) override {
		return edges[i].begin();
	}

	inline std::unordered_set<int>::const_iterator endNeighbours(int i) override {
		return edges[i].end();
	}

	inline real_t getSurfaceTension(int i, int j) override {
		if (specificParams.surfaceTensionMultiplier == 1.0) return 1.0;
		
		// If the particles are next-nearest neighbours (i.e. at least one vertex in the intersection of their neighbour sets), apply multiplier
		for (auto it1 = edges[i].begin(); it1 != edges[i].end(); it1++) {
			for (auto it2 = edges[j].begin(); it2 != edges[j].end(); it2++) {
				if (*it1 == *it2) {
					return specificParams.surfaceTensionMultiplier;
				}
			}
		}
		return 1.0;
	}
	
	inline real_t getVolume() override {
		// compute volume enclosed within the surface
		real_t volume = 0;
		int triangleCount = int(triangles.size());
		#pragma omp parallel for reduction(+: volume)
		for (int i = 0; i < triangleCount; ++i) {
			const Vec3& a = particles[triangles[i].X()].position;
			const Vec3& b = particles[triangles[i].Y()].position;
			const Vec3& c = particles[triangles[i].Z()].position;
			// compute signed volume of triangle
			real_t vCBA = c.X() * b.Y() * a.Z();
			real_t vBCA = b.X() * c.Y() * a.Z();
			real_t vCAB = c.X() * a.Y() * b.Z();
			real_t vACB = a.X() * c.Y() * b.Z();
			real_t vBAC = b.X() * a.Y() * c.Z();
			real_t vABC = a.X() * b.Y() * c.Z();
			volume += (-vCBA + vBCA + vCAB - vACB - vBAC + vABC) / real_t(6);
		}
		return volume;
	}
	
	inline std::string getTypeHint() override {
		return "s3";
	}

public:
	
	Surface3(Params params, SpecificParams specificParams, int seed = time(nullptr));

	/// Adds a particle in a random location on the surface
	void addParticle() override;

	/// Add specific info to the json string
	void specificJson(std::string& json) override;

	/// Add specific info to the binary stream
	void specificBinary(bio::BufferedBinaryFileOutput<>& data) override;

private:

	/// Adds a particle between any two existing particles and connects to neighbouring triangles
	void addParticleEdge();

	/// Adds a particle in a random location on the surface, using Delaunay triangulation
	void addParticleDelaunay();

	/// Adds a particle on an aligned edge (anisotropic growth), using Delaunay trigulation
	void addParticleEdgeDelaunay();

};
