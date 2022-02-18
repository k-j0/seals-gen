#pragma once

#include "Vec.h"

/// Represents a single vertex-particle on the evolving 3D selfavoiding surface
struct Particle {

	Vec3 acceleration; // Acceleration in 3D space

	Vec3 velocity; // Velocity in 3D space

	Vec3 position; // Position in 3D space

	Vec3 spherical; // Position on unit sphere (for spherical Delaunay triangulation)

	double noise = 0.0; // -1..1 random noise

	/// Helper factory to construct new particles
	static inline Particle FromPosition(Vec3 pos) {
		Particle p;
		p.position = pos;
		p.velocity = p.acceleration = p.spherical = Vec3::Zero();
		return p;
	}

	/// Helper factor to construct Zero particle
	static inline Particle Zero() {
		Particle p;
		p.position = p.velocity = p.acceleration = p.spherical = Vec3::Zero();
		return p;
	}

};
