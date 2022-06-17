#pragma once

#include "BoundaryCondition.h"

/// Represents a soft spherical boundary
template<int D>
class SphereBoundary : public BoundaryCondition<D> {

	// Radius of the sphere encasing the particles
	double radius;

	// How far into the sphere the force applies, in 0..1
	// Setting to 0 makes the boundary hard (i.e. only particles outside the boundary will be pushed inwards)
	double extent;

public:

	SphereBoundary(double radius = 1.0, double extent = .05) : radius(radius), extent(extent) {}

	inline Vec<double, D> force(const Vec<double, D>& position) override {
		const double posLen = sqrt(position.lengthSqr());
		if (posLen > radius * (1.0 - extent)) {
			double d = (1.0 - extent) - posLen / radius;
			return position * -d * d * .5;
		}
		return Vec<double, D>::Zero();
	}

	inline void hard(Vec<double, D>& position) override {
		if (position.lengthSqr() > radius * radius) {
			position.normalize();
			position.multiply(radius);
		}
	}

	inline std::string toJson() override {
		return "{ 'type': 'sphere', 'radius': " + std::to_string(radius) + ", 'extent': " + std::to_string(extent) + " }";
	}

};
