#pragma once

#include <string>
#include "Vec.h"

/// Represents the physical boundaries that restrict particle movement in 3D space
template<int D>
class BoundaryCondition {

public:

	virtual ~BoundaryCondition() {};

	// Returns the acceleration vector pushing the particle away from the boundary, if applicable
	virtual Vec<double, D> force(const Vec<double, D>& position) = 0;

	// Updates the position given to prevent it ever falling outside the boundary
	// (this may happen if it's pushed outwards more than the force() can bring it in)
	virtual void hard(Vec<double, D>& position) = 0;

	// Returns a json representation of the boundary condition
	virtual std::string toJson() = 0;

};
