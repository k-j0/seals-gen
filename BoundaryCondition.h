#pragma once

#include <string>
#include "Vec.h"
#include "Particle.h"
#include "BinaryIO.h"

/// Represents the physical boundaries that restrict particle movement in 3D space
template<int D>
class BoundaryCondition {
	
public:
	
	virtual ~BoundaryCondition() {};
	
	// Returns whether the boundary condition requires the enclosed surface's volume to proceed
	virtual bool needsVolume() = 0;
	
	// Optional update routine, called before each individual sim step
	virtual void update(real_t surfaceVolume) = 0;
	
	// Process a particle that is meant to be kept attached to the boundary wall
	virtual void updateAttachedParticle(Particle<D>* particle, real_t maximumAllowedDisplacement) = 0;
	
	// Returns the acceleration vector pushing the particle away from the boundary, if applicable
	virtual Vec<real_t, D> force(const Vec<real_t, D>& position) = 0;
	
	// Updates the position given to prevent it ever falling outside the boundary
	// (this may happen if it's pushed outwards more than the force() can bring it in)
	virtual void hard(Vec<real_t, D>& position) = 0;
	
	// Returns a json representation of the boundary condition
	virtual std::string toJson() = 0;
	
	// Appends a binary representation of the boundary condition to the data stream
	virtual void toBinary(bio::BufferedBinaryFileOutput<>& data) = 0;
	
};
