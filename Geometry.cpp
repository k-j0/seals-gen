#include "Geometry.h"

GeometryPtr Geometry::Icosahedron(double radius) {

	GeometryPtr geo = GeometryPtr(new Geometry);

	// see https://github.com/mrdoob/three.js/blob/master/src/geometries/IcosahedronGeometry.js

	double t = (1.0 + sqrt(5.0)) * 0.5;

	// @todo: hard-coded the first vertex as x = -1 -> x = 0 to place at exact north pole for DT - this makes the geometry no longer an icosahedron
	geo->vertices.push_back(Vec3( 0.0,    t,  0.0).normalized() * radius);
	geo->vertices.push_back(Vec3( 1.0,    t,  0.0).normalized() * radius);
	geo->vertices.push_back(Vec3(-1.0,   -t,  0.0).normalized() * radius);
	geo->vertices.push_back(Vec3( 1.0,   -t,  0.0).normalized() * radius);
	geo->vertices.push_back(Vec3( 0.0, -1.0,    t).normalized() * radius);
	geo->vertices.push_back(Vec3( 0.0,  1.0,    t).normalized() * radius);
	geo->vertices.push_back(Vec3( 0.0, -1.0,   -t).normalized() * radius);
	geo->vertices.push_back(Vec3( 0.0,  1.0,   -t).normalized() * radius);
	geo->vertices.push_back(Vec3(   t,  0.0, -1.0).normalized() * radius);
	geo->vertices.push_back(Vec3(   t,  0.0,  1.0).normalized() * radius);
	geo->vertices.push_back(Vec3(  -t,  0.0, -1.0).normalized() * radius);
	geo->vertices.push_back(Vec3(  -t,  0.0,  1.0).normalized() * radius);

	geo->indices.push_back(IVec3( 0, 11,  5));
	geo->indices.push_back(IVec3( 0,  5,  1));
	geo->indices.push_back(IVec3( 0,  1,  7));
	geo->indices.push_back(IVec3( 0,  7, 10));
	geo->indices.push_back(IVec3( 0, 10, 11));
	geo->indices.push_back(IVec3( 1,  5,  9));
	geo->indices.push_back(IVec3( 5, 11,  4));
	geo->indices.push_back(IVec3(11, 10,  2));
	geo->indices.push_back(IVec3(10,  7,  6));
	geo->indices.push_back(IVec3( 7,  1,  8));
	geo->indices.push_back(IVec3( 3,  9,  4));
	geo->indices.push_back(IVec3( 3,  4,  2));
	geo->indices.push_back(IVec3( 3,  2,  6));
	geo->indices.push_back(IVec3( 3,  6,  8));
	geo->indices.push_back(IVec3( 3,  8,  9));
	geo->indices.push_back(IVec3( 4,  9,  5));
	geo->indices.push_back(IVec3( 2,  4, 11));
	geo->indices.push_back(IVec3( 6,  2, 10));
	geo->indices.push_back(IVec3( 8,  6,  7));
	geo->indices.push_back(IVec3( 9,  8,  1));

	return geo;
}
