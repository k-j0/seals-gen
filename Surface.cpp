
#include "Surface.h"

#include "Geometry.h"


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

void Surface::addParticle() {

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

void Surface::update () {

	// update acceleration values for all particles first without writing to position
	for (std::size_t i = 0; i < particles.size(); ++i) {

		// dampen acceleration
		particles[i].acceleration.multiply(params.damping * params.damping);

		// boundary restriction (in sphere of radius params.boundaryRadius)
		const double posLen = sqrt(particles[i].position.lengthSqr());
		if (posLen > params.boundaryRadius * (1.0 - params.boundaryExtent)) {
			double d = (1.0 - params.boundaryExtent) - posLen / params.boundaryRadius;
			particles[i].acceleration.add(particles[i].position * -d * d * .5);
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
				particles[i].acceleration.add(towards);
			}
		}

		// iterate over neighbour particles
		Vec3 neighbourAvg(0, 0, 0);
		for (const int& neighbour : edges[i]) {

			neighbourAvg.add(particles[neighbour].position);

			// attract if far, repel if too close
			Vec3 towards = particles[neighbour].position - particles[i].position;
			double d = sqrt(towards.lengthSqr());
			towards.normalize();
			towards.multiply(d - params.attractionMagnitude);
			particles[i].acceleration.add(towards);

		}

		// apply rigidity rule
		neighbourAvg.multiply(1.0 / edges[i].size());
		Vec3 twdAvg = neighbourAvg - particles[i].position;
		twdAvg.normalize();
		twdAvg.multiply(params.rigidity);
		particles[i].acceleration.add(twdAvg);

	}

	// update positions for all particles
	for (std::size_t i = 0; i < particles.size(); ++i) {

		// dampen velocity
		particles[i].velocity.multiply(params.damping);

		// apply acceleration
		particles[i].velocity.add(particles[i].acceleration * params.dt);

		// apply velocity
		particles[i].position.add(particles[i].velocity * params.dt);

		// apply hard boundary
		if (particles[i].position.lengthSqr() > params.boundaryRadius * params.boundaryRadius) {
			particles[i].position.normalize();
			particles[i].position.multiply(params.boundaryRadius);
		}
	}

	++t;
}

std::string Surface::toJson() {

	std::string json = "{\n"
		"\t'date': " + std::to_string(time(nullptr)) + ",\n"
		"\t'seed': " + std::to_string(seed) + ",\n"
		"\t'timesteps': " + std::to_string(t) + ",\n";
	
	json += "\t'particles': [\n";
	for (std::size_t i = 0; i < particles.size(); ++i) {
		json += "\t\t{\n";
		json += "\t\t\t'position': " + particles[i].position.toString() + ",\n";
		json += "\t\t\t'velocity': " + particles[i].velocity.toString() + ",\n";
		json += "\t\t\t'acceleration': " + particles[i].acceleration.toString() + "\n";
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
