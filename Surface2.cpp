
#include "Surface2.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Surface2::Surface2(Params params, SpecificParams specificParams, int seed) : Surface<2, std::array<int, 2>::const_iterator>(params, seed), specificParams(specificParams) {
	
	// build initial equilateral triangle with side length = attraction magnitude
	double radius = params.attractionMagnitude;
	particles.push_back(Particle<2>::FromPosition(Vec2(radius, 0.0)));
	particles.push_back(Particle<2>::FromPosition(Vec2(std::cos(2 * M_PI / 3) * radius, std::sin(2 * M_PI / 3) * radius)));
	particles.push_back(Particle<2>::FromPosition(Vec2(std::cos(4 * M_PI / 3) * radius, std::sin(4 * M_PI / 3) * radius)));
	
	// init edges
	neighbourIndices.push_back({ 2, 1 });
	neighbourIndices.push_back({ 0, 2 });
	neighbourIndices.push_back({ 1, 0 });

	for (std::size_t i = 0; i < particles.size(); ++i) {
		addParticleToGrid(i);
	}
}

void Surface2::addParticle() {

	// pick a random particle to insert the new particle after
	int a = int(rand01() * particles.size());
	int b = neighbourIndices[a][1];

	// insert new particle
	int c = particles.size();
	particles.push_back(Particle<2>::FromPosition((particles[a].position + particles[b].position) * 0.5));
	neighbourIndices.push_back({ a, b });

	// update neighbour indices
	neighbourIndices[a][1] = c;
	neighbourIndices[b][0] = c;

	addParticleToGrid(c);
}

void Surface2::specificJson(std::string& json) {

	json += "\t'particles': [\n";
	for (std::size_t i = 0; i < particles.size(); ++i) {
		json += "\t\t{\n";
		json += "\t\t\t'position': " + particles[i].position.toString() + ",\n";
		json += "\t\t\t'velocity': " + particles[i].velocity.toString() + ",\n";
		json += "\t\t\t'acceleration': " + particles[i].acceleration.toString() + ",\n";
		json += "\t\t\t'noise': " + std::to_string(particles[i].noise) + ",\n";
		json += "\t\t\t'next': " + std::to_string(neighbourIndices[i][1]) + ",\n";
		json += "\t\t\t'previous': " + std::to_string(neighbourIndices[i][0]) + "\n";
		json += "\t\t}";
		if (i < particles.size() - 1) json += ",";
		json += "\n";
	}
	json += "\t]";
}

void Surface2::specificBinary(std::vector<uint8_t>& data) {}
