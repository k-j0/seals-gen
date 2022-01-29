#pragma once

#include "Vec.h"

/// Represents a single vertex-particle on the evolving 3D selfavoiding surface
struct Particle {

	Vec3 acceleration; // Acceleration in 3D space

	Vec3 velocity; // Velocity in 3D space

	Vec3 position; // Position in 3D space

	/// Helper factory to construct new particles
	static inline Particle FromPosition(Vec3 pos) {
		Particle p;
		p.position = pos;
		p.velocity = Vec3::Zero();
		p.acceleration = Vec3::Zero();
		return p;

	}

};
