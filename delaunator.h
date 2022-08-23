

// Adapted from https://github.com/delfrrr/delaunator-cpp (c1521f6)


#pragma once

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <utility>
#include <vector>
#include <tuple>

#include "real.h"

namespace delaunator {

    //@see https://stackoverflow.com/questions/33333363/built-in-mod-vs-custom-mod-function-improve-the-performance-of-modulus-op/33333636#33333636
    inline size_t fast_mod(const size_t i, const size_t c) {
        return i >= c ? i % c : i;
    }

    // Kahan and Babuska summation, Neumaier variant; accumulates less FP error
    inline real_t sum(const std::vector<real_t>& x) {
        real_t sum = x[0];
        real_t err = 0.0;

        for (size_t i = 1; i < x.size(); i++) {
            const real_t k = x[i];
            const real_t m = sum + k;
            err += std::fabs(sum) >= std::fabs(k) ? sum - m + k : k - m + sum;
            sum = m;
        }
        return sum + err;
    }

    inline real_t dist(
        const real_t ax,
        const real_t ay,
        const real_t bx,
        const real_t by) {
        const real_t dx = ax - bx;
        const real_t dy = ay - by;
        return dx * dx + dy * dy;
    }

    inline real_t circumradius(
        const real_t ax,
        const real_t ay,
        const real_t bx,
        const real_t by,
        const real_t cx,
        const real_t cy) {
        const real_t dx = bx - ax;
        const real_t dy = by - ay;
        const real_t ex = cx - ax;
        const real_t ey = cy - ay;

        const real_t bl = dx * dx + dy * dy;
        const real_t cl = ex * ex + ey * ey;
        const real_t d = dx * ey - dy * ex;

        const real_t x = (ey * bl - dy * cl) * (real_t)0.5 / d;
        const real_t y = (dx * cl - ex * bl) * (real_t)0.5 / d;

        if ((bl > 0.0 || bl < 0.0) && (cl > 0.0 || cl < 0.0) && (d > 0.0 || d < 0.0)) {
            return x * x + y * y;
        } else {
            return std::numeric_limits<real_t>::max();
        }
    }

    inline bool orient(
        const real_t px,
        const real_t py,
        const real_t qx,
        const real_t qy,
        const real_t rx,
        const real_t ry) {
        return (qy - py) * (rx - qx) - (qx - px) * (ry - qy) < 0.0;
    }

    inline std::pair<real_t, real_t> circumcenter(
        const real_t ax,
        const real_t ay,
        const real_t bx,
        const real_t by,
        const real_t cx,
        const real_t cy) {
        const real_t dx = bx - ax;
        const real_t dy = by - ay;
        const real_t ex = cx - ax;
        const real_t ey = cy - ay;

        const real_t bl = dx * dx + dy * dy;
        const real_t cl = ex * ex + ey * ey;
        const real_t d = dx * ey - dy * ex;

        const real_t x = ax + (ey * bl - dy * cl) * (real_t)0.5 / d;
        const real_t y = ay + (dx * cl - ex * bl) * (real_t)0.5 / d;

        return std::make_pair(x, y);
    }

    struct compare {

        std::vector<real_t> const& coords;
        real_t cx;
        real_t cy;

        bool operator()(std::size_t i, std::size_t j) {
            const real_t d1 = dist(coords[2 * i], coords[2 * i + 1], cx, cy);
            const real_t d2 = dist(coords[2 * j], coords[2 * j + 1], cx, cy);
            const real_t diff1 = d1 - d2;
            const real_t diff2 = coords[2 * i] - coords[2 * j];
            const real_t diff3 = coords[2 * i + 1] - coords[2 * j + 1];

            if (diff1 > 0.0 || diff1 < 0.0) {
                return diff1 < 0;
            } else if (diff2 > 0.0 || diff2 < 0.0) {
                return diff2 < 0;
            } else {
                return diff3 < 0;
            }
        }
    };

    inline bool in_circle(
        const real_t ax,
        const real_t ay,
        const real_t bx,
        const real_t by,
        const real_t cx,
        const real_t cy,
        const real_t px,
        const real_t py) {
        const real_t dx = ax - px;
        const real_t dy = ay - py;
        const real_t ex = bx - px;
        const real_t ey = by - py;
        const real_t fx = cx - px;
        const real_t fy = cy - py;

        const real_t ap = dx * dx + dy * dy;
        const real_t bp = ex * ex + ey * ey;
        const real_t cp = fx * fx + fy * fy;

        return (dx * (ey * cp - bp * fy) -
            dy * (ex * cp - bp * fx) +
            ap * (ex * fy - ey * fx)) < 0.0;
    }

    constexpr real_t EPSILON = std::numeric_limits<real_t>::epsilon();
    constexpr std::size_t INVALID_INDEX = std::numeric_limits<std::size_t>::max();

    inline bool check_pts_equal(real_t x1, real_t y1, real_t x2, real_t y2) {
        return std::fabs(x1 - x2) <= EPSILON &&
            std::fabs(y1 - y2) <= EPSILON;
    }

    // monotonically increases with real angle, but doesn't need expensive trigonometry
    inline real_t pseudo_angle(const real_t dx, const real_t dy) {
        const real_t p = dx / (std::abs(dx) + std::abs(dy));
        return (dy > 0.0 ? (real_t)3 - p : (real_t)1 + p) / (real_t)4; // [0..1)
    }

    struct DelaunatorPoint {
        std::size_t i;
        real_t x;
        real_t y;
        std::size_t t;
        std::size_t prev;
        std::size_t next;
        bool removed;
    };

    class Delaunator {

    public:
        std::vector<real_t> const& coords;
        std::vector<std::size_t> triangles;
        std::vector<std::size_t> halfedges;
        std::vector<std::size_t> hull_prev;
        std::vector<std::size_t> hull_next;
        std::vector<std::size_t> hull_tri;
        std::size_t hull_start;

        Delaunator(std::vector<real_t> const& in_coords);

        real_t get_hull_area();

    private:
        std::vector<std::size_t> m_hash;
        real_t m_center_x;
        real_t m_center_y;
        std::size_t m_hash_size;
        std::vector<std::size_t> m_edge_stack;

        std::size_t legalize(std::size_t a);
        std::size_t hash_key(real_t x, real_t y) const;
        std::size_t add_triangle(
            std::size_t i0,
            std::size_t i1,
            std::size_t i2,
            std::size_t a,
            std::size_t b,
            std::size_t c);
        void link(std::size_t a, std::size_t b);
    };

    Delaunator::Delaunator(std::vector<real_t> const& in_coords)
        : coords(in_coords),
        triangles(),
        halfedges(),
        hull_prev(),
        hull_next(),
        hull_tri(),
        hull_start(),
        m_hash(),
        m_center_x(),
        m_center_y(),
        m_hash_size(),
        m_edge_stack() {
        std::size_t n = coords.size() >> 1;

        real_t max_x = std::numeric_limits<real_t>::min();
        real_t max_y = std::numeric_limits<real_t>::min();
        real_t min_x = std::numeric_limits<real_t>::max();
        real_t min_y = std::numeric_limits<real_t>::max();
        std::vector<std::size_t> ids;
        ids.reserve(n);

        for (std::size_t i = 0; i < n; i++) {
            const real_t x = coords[2 * i];
            const real_t y = coords[2 * i + 1];

            if (x < min_x) min_x = x;
            if (y < min_y) min_y = y;
            if (x > max_x) max_x = x;
            if (y > max_y) max_y = y;

            ids.push_back(i);
        }
        const real_t cx = (min_x + max_x) / 2;
        const real_t cy = (min_y + max_y) / 2;
        real_t min_dist = std::numeric_limits<real_t>::max();

        std::size_t i0 = INVALID_INDEX;
        std::size_t i1 = INVALID_INDEX;
        std::size_t i2 = INVALID_INDEX;

        // pick a seed point close to the centroid
        for (std::size_t i = 0; i < n; i++) {
            const real_t d = dist(cx, cy, coords[2 * i], coords[2 * i + 1]);
            if (d < min_dist) {
                i0 = i;
                min_dist = d;
            }
        }

        const real_t i0x = coords[2 * i0];
        const real_t i0y = coords[2 * i0 + 1];

        min_dist = std::numeric_limits<real_t>::max();

        // find the point closest to the seed
        for (std::size_t i = 0; i < n; i++) {
            if (i == i0) continue;
            const real_t d = dist(i0x, i0y, coords[2 * i], coords[2 * i + 1]);
            if (d < min_dist && d > 0.0) {
                i1 = i;
                min_dist = d;
            }
        }

        real_t i1x = coords[2 * i1];
        real_t i1y = coords[2 * i1 + 1];

        real_t min_radius = std::numeric_limits<real_t>::max();

        // find the third point which forms the smallest circumcircle with the first two
        for (std::size_t i = 0; i < n; i++) {
            if (i == i0 || i == i1) continue;

            const real_t r = circumradius(
                i0x, i0y, i1x, i1y, coords[2 * i], coords[2 * i + 1]);

            if (r < min_radius) {
                i2 = i;
                min_radius = r;
            }
        }

        if (!(min_radius < std::numeric_limits<real_t>::max())) {
            throw std::runtime_error("not triangulation");
        }

        real_t i2x = coords[2 * i2];
        real_t i2y = coords[2 * i2 + 1];

        if (orient(i0x, i0y, i1x, i1y, i2x, i2y)) {
            std::swap(i1, i2);
            std::swap(i1x, i2x);
            std::swap(i1y, i2y);
        }

        std::tie(m_center_x, m_center_y) = circumcenter(i0x, i0y, i1x, i1y, i2x, i2y);

        // sort the points by distance from the seed triangle circumcenter
        std::sort(ids.begin(), ids.end(), compare{ coords, m_center_x, m_center_y });

        // initialize a hash table for storing edges of the advancing convex hull
        m_hash_size = static_cast<std::size_t>(std::llround(std::ceil(std::sqrt(n))));
        m_hash.resize(m_hash_size);
        std::fill(m_hash.begin(), m_hash.end(), INVALID_INDEX);

        // initialize arrays for tracking the edges of the advancing convex hull
        hull_prev.resize(n);
        hull_next.resize(n);
        hull_tri.resize(n);

        hull_start = i0;

        size_t hull_size = 3;

        hull_next[i0] = hull_prev[i2] = i1;
        hull_next[i1] = hull_prev[i0] = i2;
        hull_next[i2] = hull_prev[i1] = i0;

        hull_tri[i0] = 0;
        hull_tri[i1] = 1;
        hull_tri[i2] = 2;

        m_hash[hash_key(i0x, i0y)] = i0;
        m_hash[hash_key(i1x, i1y)] = i1;
        m_hash[hash_key(i2x, i2y)] = i2;

        std::size_t max_triangles = n < 3 ? 1 : 2 * n - 5;
        triangles.reserve(max_triangles * 3);
        halfedges.reserve(max_triangles * 3);
        add_triangle(i0, i1, i2, INVALID_INDEX, INVALID_INDEX, INVALID_INDEX);
        real_t xp = std::numeric_limits<real_t>::quiet_NaN();
        real_t yp = std::numeric_limits<real_t>::quiet_NaN();
        for (std::size_t k = 0; k < n; k++) {
            const std::size_t i = ids[k];
            const real_t x = coords[2 * i];
            const real_t y = coords[2 * i + 1];

            // skip near-duplicate points
            if (k > 0 && check_pts_equal(x, y, xp, yp)) continue;
            xp = x;
            yp = y;

            // skip seed triangle points
            if (
                check_pts_equal(x, y, i0x, i0y) ||
                check_pts_equal(x, y, i1x, i1y) ||
                check_pts_equal(x, y, i2x, i2y)) continue;

            // find a visible edge on the convex hull using edge hash
            std::size_t start = 0;

            size_t key = hash_key(x, y);
            for (size_t j = 0; j < m_hash_size; j++) {
                start = m_hash[fast_mod(key + j, m_hash_size)];
                if (start != INVALID_INDEX && start != hull_next[start]) break;
            }

            start = hull_prev[start];
            size_t e = start;
            size_t q;

            while (q = hull_next[e], !orient(x, y, coords[2 * e], coords[2 * e + 1], coords[2 * q], coords[2 * q + 1])) { //TODO: does it works in a same way as in JS
                e = q;
                if (e == start) {
                    e = INVALID_INDEX;
                    break;
                }
            }

            if (e == INVALID_INDEX) continue; // likely a near-duplicate point; skip it

            // add the first triangle from the point
            std::size_t t = add_triangle(
                e,
                i,
                hull_next[e],
                INVALID_INDEX,
                INVALID_INDEX,
                hull_tri[e]);

            hull_tri[i] = legalize(t + 2);
            hull_tri[e] = t;
            hull_size++;

            // walk forward through the hull, adding more triangles and flipping recursively
            std::size_t next = hull_next[e];
            while (
                q = hull_next[next],
                orient(x, y, coords[2 * next], coords[2 * next + 1], coords[2 * q], coords[2 * q + 1])) {
                t = add_triangle(next, i, q, hull_tri[i], INVALID_INDEX, hull_tri[next]);
                hull_tri[i] = legalize(t + 2);
                hull_next[next] = next; // mark as removed
                hull_size--;
                next = q;
            }

            // walk backward from the other side, adding more triangles and flipping
            if (e == start) {
                while (
                    q = hull_prev[e],
                    orient(x, y, coords[2 * q], coords[2 * q + 1], coords[2 * e], coords[2 * e + 1])) {
                    t = add_triangle(q, i, e, INVALID_INDEX, hull_tri[e], hull_tri[q]);
                    legalize(t + 2);
                    hull_tri[q] = t;
                    hull_next[e] = e; // mark as removed
                    hull_size--;
                    e = q;
                }
            }

            // update the hull indices
            hull_prev[i] = e;
            hull_start = e;
            hull_prev[next] = i;
            hull_next[e] = i;
            hull_next[i] = next;

            m_hash[hash_key(x, y)] = i;
            m_hash[hash_key(coords[2 * e], coords[2 * e + 1])] = e;
        }
    }

    real_t Delaunator::get_hull_area() {
        std::vector<real_t> hull_area;
        size_t e = hull_start;
        do {
            hull_area.push_back((coords[2 * e] - coords[2 * hull_prev[e]]) * (coords[2 * e + 1] + coords[2 * hull_prev[e] + 1]));
            e = hull_next[e];
        } while (e != hull_start);
        return sum(hull_area);
    }

    std::size_t Delaunator::legalize(std::size_t a) {
        std::size_t i = 0;
        std::size_t ar = 0;
        m_edge_stack.clear();

        // recursion eliminated with a fixed-size stack
        while (true) {
            const size_t b = halfedges[a];

            /* if the pair of triangles doesn't satisfy the Delaunay condition
            * (p1 is inside the circumcircle of [p0, pl, pr]), flip them,
            * then do the same check/flip recursively for the new pair of triangles
            *
            *           pl                    pl
            *          /||\                  /  \
            *       al/ || \bl            al/    \a
            *        /  ||  \              /      \
            *       /  a||b  \    flip    /___ar___\
            *     p0\   ||   /p1   =>   p0\---bl---/p1
            *        \  ||  /              \      /
            *       ar\ || /br             b\    /br
            *          \||/                  \  /
            *           pr                    pr
            */
            const size_t a0 = 3 * (a / 3);
            ar = a0 + (a + 2) % 3;

            if (b == INVALID_INDEX) {
                if (i > 0) {
                    i--;
                    a = m_edge_stack[i];
                    continue;
                } else {
                    //i = INVALID_INDEX;
                    break;
                }
            }

            const size_t b0 = 3 * (b / 3);
            const size_t al = a0 + (a + 1) % 3;
            const size_t bl = b0 + (b + 2) % 3;

            const std::size_t p0 = triangles[ar];
            const std::size_t pr = triangles[a];
            const std::size_t pl = triangles[al];
            const std::size_t p1 = triangles[bl];

            const bool illegal = in_circle(
                coords[2 * p0],
                coords[2 * p0 + 1],
                coords[2 * pr],
                coords[2 * pr + 1],
                coords[2 * pl],
                coords[2 * pl + 1],
                coords[2 * p1],
                coords[2 * p1 + 1]);

            if (illegal) {
                triangles[a] = p1;
                triangles[b] = p0;

                auto hbl = halfedges[bl];

                // edge swapped on the other side of the hull (rare); fix the halfedge reference
                if (hbl == INVALID_INDEX) {
                    std::size_t e = hull_start;
                    do {
                        if (hull_tri[e] == bl) {
                            hull_tri[e] = a;
                            break;
                        }
                        e = hull_next[e];
                    } while (e != hull_start);
                }
                link(a, hbl);
                link(b, halfedges[ar]);
                link(ar, bl);
                std::size_t br = b0 + (b + 1) % 3;

                if (i < m_edge_stack.size()) {
                    m_edge_stack[i] = br;
                } else {
                    m_edge_stack.push_back(br);
                }
                i++;

            } else {
                if (i > 0) {
                    i--;
                    a = m_edge_stack[i];
                    continue;
                } else {
                    break;
                }
            }
        }
        return ar;
    }

    inline std::size_t Delaunator::hash_key(const real_t x, const real_t y) const {
        const real_t dx = x - m_center_x;
        const real_t dy = y - m_center_y;
        return fast_mod(
            static_cast<std::size_t>(std::llround(std::floor(pseudo_angle(dx, dy) * static_cast<real_t>(m_hash_size)))),
            m_hash_size);
    }

    std::size_t Delaunator::add_triangle(
        std::size_t i0,
        std::size_t i1,
        std::size_t i2,
        std::size_t a,
        std::size_t b,
        std::size_t c) {
        std::size_t t = triangles.size();
        triangles.push_back(i0);
        triangles.push_back(i1);
        triangles.push_back(i2);
        link(t, a);
        link(t + 1, b);
        link(t + 2, c);
        return t;
    }

    void Delaunator::link(const std::size_t a, const std::size_t b) {
        std::size_t s = halfedges.size();
        if (a == s) {
            halfedges.push_back(b);
        } else if (a < s) {
            halfedges[a] = b;
        } else {
            throw std::runtime_error("Cannot link edge");
        }
        if (b != INVALID_INDEX) {
            std::size_t s2 = halfedges.size();
            if (b == s2) {
                halfedges.push_back(a);
            } else if (b < s2) {
                halfedges[b] = a;
            } else {
                throw std::runtime_error("Cannot link edge");
            }
        }
    }

} //namespace delaunator
