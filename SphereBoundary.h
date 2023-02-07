#pragma once

#include "BoundaryCondition.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
	
	real_t targetVolumeFraction;
    
    bool withOffset = false;
	
public:
	
	SphereBoundary(real_t radius = 1.0, real_t maxRadius = 1.0, real_t extent = .05, real_t growthRate = 1.0, real_t targetVolumeFraction = 0.0, bool withOffset = false) :
		radius(radius), maxRadius(maxRadius), extent(extent), growthRate(growthRate), targetVolumeFraction(targetVolumeFraction), withOffset(withOffset) {}
	
	inline bool needsVolume () override {
		return targetVolumeFraction > 0;
	}
	
	inline void update(real_t surfaceVolume) override {
		if (targetVolumeFraction > 0) {
			real_t volume = M_PI * radius * radius;
			real_t currentVolumeFraction = surfaceVolume / volume;
			if (currentVolumeFraction > targetVolumeFraction) {
				// grow to accomodate the inner volume - never shrink
				radius = std::sqrt(surfaceVolume / (targetVolumeFraction * M_PI)); // solve for the radius in reverse, swapping out currentVolumeFraction for targetVolumeFraction
				radius = radius > maxRadius ? maxRadius : radius;
			}
		} else if (growthRate > 1) {
			radius *= growthRate;
			radius = radius > maxRadius ? maxRadius : radius;
		}
	}
	
	void updateAttachedParticles(std::vector<Particle<D>>& particles, real_t maximumAllowedDisplacement) override {
        Particle<D>* particle = &particles[0];
        if (particle->attached) {
            if (withOffset) {
                // move the whole set of particles relative to the first so that the leftmost point on X on the boundary is stuck to the first particle, no matter where it goes
                Vec<real_t, D> offset = -particle->position;
                offset.setX(offset.X() - radius);
                int numParticles = int(particles.size());
                #pragma omp parallel for
                for (int i = 0; i < numParticles; ++i) {
                    particles[i].position += offset;
                }
            } else {
                // move the particle towards the leftmost point on X on the boudnary
                Vec<real_t, D> target = Vec<real_t, D>::Zero();
                target.setX(-radius);
                particle->position.moveTowards(target, maximumAllowedDisplacement);
            }
        }
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
			position *= radius;
		}
	}
	
	inline std::string toJson() override {
		return "{ 'type': 'sphere', 'radius': " + std::to_string(radius) + ", 'extent': " + std::to_string(extent) + " }";
	}
	
	inline void toBinary(bio::BufferedBinaryFileOutput<>& data) override {
		bio::writeSimple<std::int8_t>(data, 0); // sphere type id = 0
		bio::writeSimple<float>(data, radius);
		bio::writeSimple<float>(data, extent);
        bio::writeSimple<bool>(data, withOffset);
	}
	
};
