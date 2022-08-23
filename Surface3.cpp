
#include "Surface3.h"

#include "Options.h"
#include "Geometry.h"
#include "SphericalDelaunay.h"
#include "warnings.h"


WARNING_DISABLE_OMP_PRAGMAS;


Surface3::Surface3(Surface3::Params params, SpecificParams specificParams, int seed) : Surface<3, std::unordered_set<int>::const_iterator>(params, seed), specificParams(specificParams) {

	// build initial geometry (icosahedron with radius = attraction magnitude)
	GeometryPtr icosahedron = Geometry::Icosahedron(params.attractionMagnitude);
	particles = std::vector<Particle<3>>(icosahedron->vertices.size());
	triangles = icosahedron->indices;
	for (std::size_t i = 0; i < particles.size(); ++i) {
		particles[i] = Particle<3>::FromPosition(icosahedron->vertices[i]);
		particles[i].noise = rand01() * 2 - 1;
		// initial particles share spherical coords with their original positions (likely never the case later)
		particles[i].spherical = particles[i].position.normalized();
	}

	// init edges amongst original geo
	edges = std::vector<std::unordered_set<int>>(particles.size());
#define CONNECT(a, b) edges[a].insert(b); edges[b].insert(a);
	for (auto it = triangles.begin(); it != triangles.end(); it++) {
		CONNECT(it->X(), it->Y());
		CONNECT(it->Y(), it->Z());
		CONNECT(it->Z(), it->X());
	}
#undef CONNECT

	for (std::size_t i = 0; i < particles.size(); ++i) {
		addParticleToGrid((int)i);
	}
}

void Surface3::addParticle() {
	switch (specificParams.strategy) {
	case GrowthStrategy::ON_EDGE:
		addParticleEdge();
		return;
	case GrowthStrategy::DELAUNAY:
		addParticleDelaunay();
		return;
	case GrowthStrategy::DELAUNAY_ANISO_EDGE:
		addParticleEdgeDelaunay();
		return;
	default:
		printf("Error: invalid growth strategy!");
		exit(1);
	}
}

void Surface3::specificJson(std::string& json) {

	json += "\t'growthStrategy': ";
	switch (specificParams.strategy) {
	case GrowthStrategy::ON_EDGE:				json += "'edge'";			break;
	case GrowthStrategy::DELAUNAY:				json += "'delaunay'";		break;
	case GrowthStrategy::DELAUNAY_ANISO_EDGE:	json += "'delaunay-aniso'";	break;
	}
	json += ",\n";

	json += "\t'particles': [\n";
	for (std::size_t i = 0; i < particles.size(); ++i) {
		json += "\t\t{\n";
		json += "\t\t\t'position': " + particles[i].position.toString() + ",\n";
		json += "\t\t\t'velocity': " + particles[i].velocity.toString() + ",\n";
		json += "\t\t\t'acceleration': " + particles[i].acceleration.toString() + ",\n";
		json += "\t\t\t'spherical': " + particles[i].spherical.toString() + ",\n";
		json += "\t\t\t'noise': " + std::to_string(particles[i].noise) + "\n";
		json += "\t\t}";
		if (i < particles.size() - 1) json += ",";
		json += "\n";
	}
	json += "\t],\n";

	json += "\t'triangles': [\n";
	for (std::size_t i = 0; i < triangles.size(); ++i) {
		json += "\t\t" + triangles[i].toString();
		if (i < triangles.size() - 1) json += ",";
		json += "\n";
	}
	json += "\t]\n";

}

void Surface3::specificBinary(std::vector<uint8_t>& data) {

	// Particle positions
	bio::writeSimple<int>(data, (int)particles.size());
	for (size_t i = 0; i < particles.size(); ++i) {
		bio::writeVec(data, particles[i].position);
	}

	// Triangle indices
	bio::writeSimple<int>(data, (int)triangles.size());
	for (size_t i = 0; i < triangles.size(); ++i) {
		bio::writeVec(data, triangles[i]);
	}

}




void Surface3::addParticleEdge() {

	// get 2 random connected particles among the existing set and insert a new one in the middle
	int a = int(rand01() * edges.size());
	int bIdx = int(rand01() * edges[a].size());
	int b = -1; // O(N log N) random access in set - done here because these sets are likely to be kept fairly small ideally
	for (auto it = edges[a].begin(); it != edges[a].end(); it++) {
		--bIdx;
		if (bIdx <= 0) {
			b = *it;
			break;
		}
	}
	assert(b > -1);
	int c = (int)particles.size();
	particles.push_back(Particle<3>::FromPosition(Vec3::Lerp(particles[a].position, particles[b].position, 0.5)));
	particles.back().noise = rand01() * 2 - 1;

	// update edge map
	edges.push_back(std::unordered_set<int>());
	edges[a].erase(b);		edges[b].erase(a);
	edges[a].insert(c);		edges[b].insert(c);
	edges[c].insert(a);		edges[c].insert(b);

	// replace any tris sharing edge [a,b] with two tris new tris on either side of C
	// @todo: this O(N) step can be replaced with an equivalent O(1) step with an edge -> triangle map
	std::size_t size = triangles.size();
	for (std::size_t i = 0; i < size; ++i) {
		auto it = &triangles[i];
		int d = it->X();
		int e = it->Y();
		int f = it->Z();
		if ((d == a && e == b) || (d == b && e == a)) { // tri with F
			it->setX(c); // replace with a, c, f
			triangles.push_back(IVec3(d, c, f)); // add b, c, f
			edges[c].insert(f);
			edges[f].insert(c);
		} else if ((e == a && f == b) || (e == b && f == a)) { // tri with D
			it->setY(c); // replace with d, c, a
			triangles.push_back(IVec3(d, e, c)); // add d, c, b
			edges[c].insert(d);
			edges[d].insert(c);
		} else if ((f == a && d == b) || (f == b && d == a)) { // tri with E
			it->setZ(c); // replace with a, e, c
			triangles.push_back(IVec3(f, c, e)); // add b, e, c
			edges[c].insert(e);
			edges[e].insert(c);
		}
	}

	addParticleToGrid((int)particles.size() - 1);

}

void Surface3::addParticleDelaunay() {

	// @todo For now, this takes around 10x as long to compute as addParticle() since the entire mesh is retriangulated

	particles.push_back(Particle<3>::Zero());
	Particle<3>& p = particles.back();

	// place particle anywhere on the unit sphere
	do {
		do {
			p.spherical = Vec3(rand01() - 0.5, rand01() - 0.5, rand01() - 0.5);
		} while (p.spherical.lengthSqr() == 0); // prevent from creating point (0,0,0)
		p.spherical.normalize();
	} while (p.spherical.Y() == 1); // prevent from placing a point at the north pole

	// update the triangulation including the new particle
	edges.push_back(std::unordered_set<int>()); // add slot for the new particle in the edge map
	sd::SphericalDelaunay(particles, triangles, edges);

	// set other fields of p to averages amongst spherical neighbours for now (will update with everything else later on)
	p.noise = rand01() * 2 - 1;
#ifndef NO_UPDATE
	for (int neighbour : edges[particles.size() - 1]) {
		p.position.add(particles[neighbour].position);
	}
	p.position.multiply((real_t)1 / edges[particles.size() - 1].size());
#else
	p.position = p.spherical;
#endif

	addParticleToGrid((int)particles.size() - 1);
}

void Surface3::addParticleEdgeDelaunay() {

	particles.push_back(Particle<3>::Zero());
	Particle<3>& p = particles.back();

	// pick two neighbouring particles with higher probability on edges aligned with Z
	int a = -1, b = -1;
	do {
		a = int(rand01() * edges.size());
		int bIdx = int(rand01() * edges[a].size());
		// O(N log N) random access in set - done here because these sets are likely to be kept fairly small ideally
		for (auto it = edges[a].begin(); it != edges[a].end(); it++) {
			--bIdx;
			if (bIdx <= 0) {
				b = *it;
				break;
			}
		}
		assert(b > -1);
		assert((size_t)a < particles.size());
		assert((size_t)b < particles.size());
		Vec3 dir = particles[a].position - particles[b].position;
		dir.normalize();
		if (rand01() < abs(dir.Z())) {
			break;
		}
	} while (true);

	// place particle between the two selected on the unit sphere
	assert(a > -1 && (size_t)a < particles.size());
	assert(b > -1 && (size_t)b < particles.size());
	p.spherical = particles[a].spherical + particles[b].spherical;
	p.spherical.normalize();

	// update the triangulation including the new particle
	edges.push_back(std::unordered_set<int>()); // add slot for the new particle in the edge map
	sd::SphericalDelaunay(particles, triangles, edges);

	// set other fields of p to averages amongst spherical neighbours for now (will update with everything else later on)
	p.noise = rand01() * 2 - 1;
#ifndef NO_UPDATE
	for (int neighbour : edges[particles.size() - 1]) {
		p.position.add(particles[neighbour].position);
	}
	p.position.multiply((real_t)1 / edges[particles.size() - 1].size());
#else
	p.position = p.spherical;
#endif

	addParticleToGrid((int)particles.size() - 1);
}
