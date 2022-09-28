#pragma once

#include "BoundaryCondition.h"

/// Represents a soft spherical boundary
template<int D>
class SphereBoundary : public BoundaryCondition<D> {

	// Radius of the sphere encasing the particles
	real_t radius;

	real_t maxRadius;

	// How far into the sphere the force applies, in 0..1
	// Setting to 0 makes the boundary hard (i.e. only particles outside the boundary will be pushed inwards)
	real_t extent;

	real_t growthRate;

public:

	SphereBoundary(real_t radius = 1.0, real_t maxRadius = 1.0, real_t extent = .05, real_t growthRate = 1.0) :
		radius(radius), maxRadius(maxRadius), extent(extent), growthRate(growthRate) {}

	inline void update() override {
		if (growthRate > 1) {
			radius *= growthRate;
			radius = radius > maxRadius ? maxRadius : radius;
		}
	}

	void updateAttachedParticle(Particle<D>* particle, real_t maximumAllowedDisplacement) override {
		Vec<real_t, D> target = particle->position.normalized();
		target.multiply(radius);
		particle->position.moveTowards(target, maximumAllowedDisplacement);
	}

	inline Vec<real_t, D> force(const Vec<real_t, D>& position) override {
		const real_t posLen = std::sqrt(position.lengthSqr());
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
