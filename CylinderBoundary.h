#pragma once

#include "BoundaryCondition.h"

#include "real.h"

class CylinderBoundary : public BoundaryCondition<3> {

	real_t radius;

	real_t extent;

public:

	CylinderBoundary(real_t radius = 1.0, real_t extent = .05) : radius(radius), extent(extent) {}

	inline Vec3 force(const Vec3& position) override {
		Vec3 f = Vec3::Zero();
		Vec2 xy = position.XY();
		const real_t posLen = sqrt(xy.lengthSqr());
		if (posLen > radius * (1.0 - extent)) {
			real_t d = ((real_t)1 - extent) - posLen / radius;
			f.setXY(xy * -d * d * (real_t).5);
		}
		return f;
	}

	inline void hard(Vec3& position) override {
		Vec2 xy = position.XY();
		if (xy.lengthSqr() > radius * radius) {
			xy.normalize();
			xy.multiply(radius);
			position.setXY(xy);
		}
	}

	inline std::string toJson() override {
		return "{ 'type': 'cylinder', 'radius': " + std::to_string(radius) + ", 'extent': " + std::to_string(extent) + " }";
	}

};
