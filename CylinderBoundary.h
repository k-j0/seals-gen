#pragma once

#include "BoundaryCondition.h"

class CylinderBoundary : public BoundaryCondition {

	double radius;

	double extent;

public:

	CylinderBoundary(double radius = 1.0, double extent = .05) : radius(radius), extent(extent) {}

	inline Vec3 force(const Vec3& position) override {
		Vec3 f = Vec3::Zero();
		Vec2 xy = position.XY();
		const double posLen = sqrt(xy.lengthSqr());
		if (posLen > radius * (1.0 - extent)) {
			double d = (1.0 - extent) - posLen / radius;
			f.setXY(xy * -d * d * .5);
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
