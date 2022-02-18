#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <unordered_set>
#include "delaunator.h"
#include "Vec.h"
#include "Particle.h"

namespace sd {

    // 3D Cartesian to spherical (deg)
    Vec2 cartesianToSpherical(Vec3 v3) {
        return Vec2(atan2(v3.Y(), v3.X()) * 180.0 / M_PI, asin(std::max(-1.0, std::min(1.0, v3.Z()))) * 180.0 / M_PI);
    }

    // Spherical (deg) to 3D cartesian
    Vec3 sphericalToCartesian(Vec2 v2) {
        const double lambda = v2.X() * M_PI / 180.0,
            phi = v2.Y() * M_PI / 180.0,
            cosphi = cos(phi);
        return Vec3(cosphi * cos(lambda), cosphi * sin(lambda), sin(phi));
    }

    /// Creates the delaunay triangulation for the set of particles
    /// Adapted from https://github.com/Fil/d3-geo-voronoi/blob/b391ee46d097f5ce41f80c1a2b8d12e34fd685ea/src/delaunay.js#L45
    void SphericalDelaunay(const std::vector<Particle>& particles, std::vector<IVec3>& outTriangles, std::vector<std::unordered_set<int>>& outEdges) {

        assert(particles.size() > 1);

        // map particles to stereographic projection
        // @todo: for now, north pole is always holed
        std::vector<double> points(particles.size() * 2); // x1 y1 x2 y2 etc
        for (std::size_t i = 0; i < particles.size(); ++i) {
            // From https://en.wikipedia.org/wiki/Stereographic_projection#First_formulation
            points[i * 2] = particles[i].spherical.X() / (1.0 - particles[i].spherical.Y());
            points[i * 2 + 1] = particles[i].spherical.Z() / (1.0 - particles[i].spherical.Y());
        }

        // Run Delaunay triangulation on projected points
        delaunator::Delaunator delaunay(points);
        
        // Apply triangles to the outTriangles array
        outTriangles.clear();
        outTriangles.reserve(delaunay.triangles.size());
        for (std::size_t i = 0; i < delaunay.triangles.size(); i += 3) {
            outTriangles.push_back(IVec3(delaunay.triangles[i], delaunay.triangles[i+1], delaunay.triangles[i+2]));
        }

        // Recompute edge map
        for (auto& edges : outEdges) {
            edges.clear(); // clear inner edge sets, leaving edges as-is in terms of length
        }
#define CONNECT(a, b) outEdges[a].insert(b); outEdges[b].insert(a);
        for (auto it = outTriangles.begin(); it != outTriangles.end(); it++) {
            CONNECT(it->X(), it->Y());
            CONNECT(it->Y(), it->Z());
            CONNECT(it->Z(), it->X());
        }
#undef CONNECT
    }

}
