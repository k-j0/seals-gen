#pragma once

#include "Vec.h"

/// Represents a single vertex-particle on the evolving n-dimensional selfavoiding surface
template<int D>
struct Particle {

	Vec<real_t, D> acceleration; // Acceleration in n-dimensional space

	Vec<real_t, D> velocity; // Velocity in n-dimensional space

	Vec<real_t, D> position; // Position in n-dimensional space

	Vec3 spherical; // Position on unit sphere (for spherical Delaunay triangulation)
    
	bool attached = false; // If true, the specific particle should be considered attached to the nearest boundary wall
    
    real_t flexibility = 1.0; // 0..1

	/// Helper factory to construct new particles
	static inline Particle FromPosition(Vec<real_t, D> pos) {
		Particle p;
		p.position = pos;
		p.velocity = p.acceleration = Vec<real_t, D>::Zero();
		p.spherical = Vec3::Zero();
		return p;
	}

	/// Helper factor to construct Zero particle
	static inline Particle Zero() {
		Particle p;
		p.position = p.velocity = p.acceleration = Vec<real_t, D>::Zero();
		p.spherical = Vec3::Zero();
		return p;
	}

};
