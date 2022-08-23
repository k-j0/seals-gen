#pragma once

#include "BoundaryCondition.h"

/// Represents a soft spherical boundary
template<int D>
class SphereBoundary : public BoundaryCondition<D> {

	// Radius of the sphere encasing the particles
	real_t radius;

	// How far into the sphere the force applies, in 0..1
	// Setting to 0 makes the boundary hard (i.e. only particles outside the boundary will be pushed inwards)
	real_t extent;

public:

	SphereBoundary(real_t radius = 1.0, real_t extent = .05) : radius(radius), extent(extent) {}

	inline Vec<real_t, D> force(const Vec<real_t, D>& position) override {
		const real_t posLen = sqrt(position.lengthSqr());
		if (posLen > radius * (1.0 - extent)) {
			real_t d = ((real_t)1 - extent) - posLen / radius;
			return position * -d * d * .5;
		}
		return Vec<real_t, D>::Zero();
	}

	inline void hard(Vec<real_t, D>& position) override {
		if (position.lengthSqr() > radius * radius) {
			position.normalize();
			position.multiply(radius);
		}
	}

	inline std::string toJson() override {
		return "{ 'type': 'sphere', 'radius': " + std::to_string(radius) + ", 'extent': " + std::to_string(extent) + " }";
	}

};
