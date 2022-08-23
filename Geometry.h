#pragma once

#include <vector>
#include <memory>
#include "Vec.h"

#include "real.h"



struct Geometry {

	/// Vertex and index buffers corresponding to a geometry
	std::vector<Vec3> vertices;
	std::vector<IVec3> indices;

	/// Generates a simple icosahedron geometry
	static std::shared_ptr<Geometry> Icosahedron(real_t radius);

};

typedef std::shared_ptr<Geometry> GeometryPtr;

