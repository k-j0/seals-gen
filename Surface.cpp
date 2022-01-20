#include "Surface.h"


#define RAND01 double(abs(int(rng())) % 10000) / 10000.0


Surface::Surface(int initialParticleCount, int seed) :
		seed (seed),
		rng (std::mt19937(seed)) {

	for (int i = 0; i < initialParticleCount; ++i) {
		addParticle();
	}

}

void Surface::addParticle() {

	particles.push_back(Particle());
	particles.back().uv = Vec2(RAND01, RAND01);

}

std::string Surface::toJson() {

	std::string json = "{\n"
		"\t'time': " + std::to_string(time(nullptr)) + ",\n"
		"\t'seed': " + std::to_string(seed) + ",\n"
		"\t'particles': [\n";

	for (std::size_t i = 0; i < particles.size(); ++i) {
		json += "\t\t{\n";
		json += "\t\t\t'uv': " + particles[i].uv.toString() + ",\n";
		json += "\t\t\t'position': " + particles[i].position.toString() + ",\n";
		json += "\t\t\t'velocity': " + particles[i].velocity.toString() + ",\n";
		json += "\t\t\t'acceleration': " + particles[i].acceleration.toString() + "\n";
		json += "\t\t}";
		if (i < particles.size() - 1) json += ",";
		json += "\n";
	}

	json += "\t]\n";

	std::replace(json.begin(), json.end(), '\'', '"');
	return json + "}";

}
