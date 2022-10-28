#pragma once

#include "BoundaryCondition.h"

#include "real.h"

class CylinderBoundary : public BoundaryCondition<3> {

	real_t radius;
	real_t maxRadius;
	real_t extent;
	real_t growthRate;

public:

	CylinderBoundary(real_t radius = 1.0, real_t maxRadius = 1.0, real_t extent = .05, real_t growthRate = 0.0) :
		radius(radius), maxRadius(maxRadius), extent(extent), growthRate(growthRate) {}

	inline void update() override {
		if (growthRate > 1) {
			radius *= growthRate;
			radius = radius > maxRadius ? maxRadius : radius;
		}
	}

	void updateAttachedParticle(Particle<3>* particle, real_t maximumAllowedDisplacement) override {
		Vec<real_t, 2> target = particle->position.XY().normalized();
		target *= radius;
		particle->position.moveTowards(Vec3(target.X(), target.Y(), particle->position.Z()), maximumAllowedDisplacement);
	}

	inline Vec3 force(const Vec3& position) override {
		Vec3 f = Vec3::Zero();
		Vec2 xy = position.XY();
		const real_t posLen = std::sqrt(xy.lengthSqr());
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
			xy *= radius;
			position.setXY(xy);
		}
	}

	inline std::string toJson() override {
		return "{ 'type': 'cylinder', 'radius': " + std::to_string(radius) + ", 'extent': " + std::to_string(extent) + " }";
	}

};
