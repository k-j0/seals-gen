
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
	}

}

void Surface::addParticle() {
	// @todo
}

std::string Surface::toJson() {

	std::string json = "{\n"
		"\t'time': " + std::to_string(time(nullptr)) + ",\n"
		"\t'seed': " + std::to_string(seed) + ",\n";
	
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
