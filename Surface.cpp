
#include "Surface.h"

#include "Geometry.h"
#include "SphericalDelaunay.h"
#include "Utils.h"


#define RAND01 double(abs(int(rng())) % 10000) / 10000.0


Surface::Surface(Surface::Params params, int seed) :
		params(params),
		seed (seed),
		rng (std::mt19937(seed)) {

	// build initial geometry (icosahedron with radius = attraction magnitude)
	GeometryPtr icosahedron = Geometry::Icosahedron(params.attractionMagnitude);
	particles = std::vector<Particle>(icosahedron->vertices.size());
	triangles = icosahedron->indices;
	for (std::size_t i = 0; i < particles.size(); ++i) {
		particles[i] = Particle::FromPosition(icosahedron->vertices[i]);
		particles[i].noise = RAND01 * 2 - 1;
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

}

void Surface::addParticle () {

	// get 2 random connected particles among the existing set and insert a new one in the middle
	int a = int(RAND01 * edges.size());
	int bIdx = int(RAND01 * edges[a].size());
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
	particles.push_back(Particle::FromPosition(Vec3::Lerp(particles[a].position, particles[b].position, 0.5)));
	particles.back().noise = RAND01 * 2 - 1;

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

}

void Surface::addParticleDelaunay () {

	// @todo For now, this takes around 10x as long to compute as addParticle() since the entire mesh is retriangulated

	particles.push_back(Particle::Zero());
	Particle& p = particles.back();

	// place particle anywhere on the unit sphere
	do {
		do {
			p.spherical = Vec3(RAND01 - 0.5, RAND01 - 0.5, RAND01 - 0.5);
		} while (p.spherical.lengthSqr() == 0); // prevent from creating point (0,0,0)
		p.spherical.normalize();
	} while (p.spherical.Y() == 1); // prevent from placing a point at the north pole

	// update the triangulation including the new particle
	edges.push_back(std::unordered_set<int>()); // add slot for the new particle in the edge map
	sd::SphericalDelaunay(particles, triangles, edges);

	// set other fields of p to averages amongst spherical neighbours for now (will update with everything else later on)
	p.noise = RAND01 * 2 - 1;
	for (int neighbour : edges[particles.size() - 1]) {
		p.position.add(particles[neighbour].position);
	}
	p.position.multiply(1.0 / edges[particles.size() - 1].size());

}

void Surface::update () {

	int numParticles = (int)particles.size();

	// update acceleration values for all particles first without writing to position
	#pragma omp parallel for
	for (int i = 0; i < numParticles; ++i) {

		// dampen acceleration
		particles[i].acceleration.multiply(params.damping * params.damping);

		// boundary restriction force
		if (params.boundary) {
			particles[i].acceleration.add(params.boundary->force(particles[i].position));
		}
		

		// iterate over non-neighbour particles
		for (std::size_t j = 0; j < particles.size(); ++j) {
			if (i == j || edges[i].find(j) != edges[i].end()) continue; // same particle, or nearest neighbours

			// repel if close enough
			Vec3 towards = particles[j].position - particles[i].position;
			double d = sqrt(towards.lengthSqr()) * (1 + particles[i].noise * params.noise);
			if (d < params.attractionMagnitude * params.repulsionMagnitudeFactor) {
				towards.normalize();
				towards.multiply(d - params.attractionMagnitude * params.repulsionMagnitudeFactor);
				particles[i].acceleration.add(towards <hadamard> params.repulsionAnisotropy);
			}
		}

		// iterate over neighbour particles
		for (const int& neighbour : edges[i]) {

			// attract if far, repel if too close
			Vec3 towards = particles[neighbour].position - particles[i].position;
			double d = sqrt(towards.lengthSqr());
			towards.normalize();
			towards.multiply(d - params.attractionMagnitude);
			particles[i].acceleration.add(towards);
		}
	}

	// update positions for all particles
	#pragma omp parallel for
	for (int i = 0; i < numParticles; ++i) {

		// dampen velocity
		particles[i].velocity.multiply(params.damping);

		// apply acceleration
		particles[i].velocity.add(particles[i].acceleration * params.dt);

		// apply velocity
		particles[i].position.add(particles[i].velocity * params.dt);

		// apply hard boundary
		if (params.boundary) {
			params.boundary->hard(particles[i].position);
		}
	}

	++t;
}

std::string Surface::toJson() {

	std::string json = "{\n"
		"\t'date': " + std::to_string(time(nullptr)) + ",\n"
		"\t'machine': '" + getMachineName() + "',\n"
		"\t'seed': " + std::to_string(seed) + ",\n"
		"\t'timesteps': " + std::to_string(t) + ",\n"
		"\t'attractionMagnitude': " + std::to_string(params.attractionMagnitude) + ",\n"
		"\t'repulsionMagnitudeFactor': " + std::to_string(params.repulsionMagnitudeFactor) + ",\n"
		"\t'damping': " + std::to_string(params.damping) + ",\n"
		"\t'noise': " + std::to_string(params.noise) + ",\n"
		"\t'repulsionAnisotropy': " + params.repulsionAnisotropy.toString() + ",\n"
		"\t'boundary': " + (params.boundary ? params.boundary->toJson() : "null") + ",\n"
		"\t'dt': " + std::to_string(params.dt) + ",\n";
	
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

	std::replace(json.begin(), json.end(), '\'', '"');
	return json + "}";

}
