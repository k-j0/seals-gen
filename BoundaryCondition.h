#pragma once

#include "Vec.h"

/// Represents the physical boundaries that restrict particle movement in 3D space
class BoundaryCondition {

public:

	// Returns the acceleration vector pushing the particle away from the boundary, if applicable
	virtual Vec3 force(const Vec3& position) = 0;

	// Updates the position given to prevent it ever falling outside the boundary
	// (this may happen if it's pushed outwards more than the force() can bring it in)
	virtual void hard(Vec3& position) = 0;

};
