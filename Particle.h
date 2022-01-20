#pragma once

#include "Vec.h"

/// Represents a single vertex-particle on the evolving 3D selfavoiding surface
struct Particle {

	Vec2 uv; // Position on 2D spherical projection of the surface (latitude/theta 0..1, longitude/phi 0..1)

	Vec3 acceleration; // Acceleration in 3D space

	Vec3 velocity; // Velocity in 3D space

	Vec3 position; // Position in 3D space

};
