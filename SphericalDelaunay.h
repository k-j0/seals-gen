#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <unordered_set>
#include "delaunator.h"
#include "Vec.h"
#include "Particle.h"

namespace sd {

    // hash function for pair<int, int>
    std::hash<int> hash;
    struct int_pair_hash {
        inline std::size_t operator()(const std::pair<int, int>& v) const {
            return hash(v.first) ^ hash(v.second);
        }
    };

    /// Creates the delaunay triangulation for the set of particles
    /// Adapted from https://github.com/Fil/d3-geo-voronoi/blob/b391ee46d097f5ce41f80c1a2b8d12e34fd685ea/src/delaunay.js#L45
    void SphericalDelaunay(const std::vector<Particle>& particles, std::vector<IVec3>& outTriangles, std::vector<std::unordered_set<int>>& outEdges) {

        assert(particles.size() > 1);

        // map particles to stereographic projection (skipping index 0 (= north pole)
        std::vector<double> points(particles.size() * 2 - 2); // x1 y1 x2 y2 etc
        for (std::size_t i = 1; i < particles.size(); ++i) {
            // From https://en.wikipedia.org/wiki/Stereographic_projection#First_formulation
            points[i * 2 - 2] = particles[i].spherical.X() / (1.0 - particles[i].spherical.Y());
            points[i * 2 - 1] = particles[i].spherical.Z() / (1.0 - particles[i].spherical.Y());
        }

        // Run Delaunay triangulation on projected points
        delaunator::Delaunator delaunay(points);
        
        // Apply triangles to the outTriangles array
        outTriangles.clear();
        outTriangles.reserve(delaunay.triangles.size());
        for (std::size_t i = 0; i < delaunay.triangles.size(); i += 3) {
            outTriangles.push_back(IVec3(delaunay.triangles[i]+1, delaunay.triangles[i+2]+1, delaunay.triangles[i+1]+1));
        }

        // Recompute edge map and find edges that aren't shared by 2 triangles
        for (auto& edges : outEdges) {
            edges.clear(); // clear inner edge sets, leaving edges as-is in terms of length
        }
        std::unordered_set<std::pair<int, int>, int_pair_hash> leftoverEdges;
#define CONNECT(a, b) \
            outEdges[a].insert(b); \
            outEdges[b].insert(a); \
            if (leftoverEdges.find(std::make_pair(a, b)) != leftoverEdges.end()) { \
                leftoverEdges.erase(std::make_pair(a, b)); \
            } else if (leftoverEdges.find(std::make_pair(b, a)) != leftoverEdges.end()) { \
                leftoverEdges.erase(std::make_pair(b, a)); \
            } else { \
                leftoverEdges.insert(std::make_pair(a, b)); \
            }
        for (auto it = outTriangles.begin(); it != outTriangles.end(); it++) {
            CONNECT(it->X(), it->Y());
            CONNECT(it->Y(), it->Z());
            CONNECT(it->Z(), it->X());
        }
#undef CONNECT

        // Re-introduce north pole vertex triangles/edges
        for (auto& edge : leftoverEdges) {
            outTriangles.push_back(IVec3(0, edge.second, edge.first));
            outEdges[0].insert(edge.first);
            outEdges[edge.first].insert(0);
            outEdges[0].insert(edge.second);
            outEdges[edge.second].insert(0);
        }
    }

}
