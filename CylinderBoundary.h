#pragma once

#include "BoundaryCondition.h"

class CylinderBoundary : public BoundaryCondition {

	double radius;

	double extent;

public:

	CylinderBoundary(double radius = 1.0, double extent = .05) : radius(radius), extent(extent) {}

	inline Vec3 force(const Vec3& position) override {
		Vec3 f = Vec3::Zero();
		Vec2 yz = position.YZ();
		const double posLen = sqrt(yz.lengthSqr());
		if (posLen > radius * (1.0 - extent)) {
			double d = (1.0 - extent) - posLen / radius;
			f.setYZ(yz * -d * d * .5);
		}
		return f;
	}

	inline void hard(Vec3& position) override {
		Vec2 yz = position.YZ();
		if (yz.lengthSqr() > radius * radius) {
			yz.normalize();
			yz.multiply(radius);
			position.setYZ(yz);
		}
	}

	inline std::string toJson() override {
		return "{ 'type': 'cylinder', 'radius': " + std::to_string(radius) + ", 'extent': " + std::to_string(extent) + " }";
	}

};
