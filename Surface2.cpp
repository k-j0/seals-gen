
#include "Surface2.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Surface2::Surface2(Params params, SpecificParams specificParams, int seed) : Surface<2, std::array<int, 2>::const_iterator>(params, seed), specificParams(specificParams) {
	
	// build initial regular n-gon with side length = attraction magnitude
	int n = specificParams.initialParticleCount;
	real_t radius = real_t(params.attractionMagnitude / (real_t(2.0) * std::sin(M_PI / n)));
	for (int i = 0; i < n; ++i) {
		real_t angle = real_t(M_PI * 2.0 * real_t(i) / n);
		real_t localRadius = radius * (1 + rand01() * specificParams.initialNoise);
		particles.push_back(Particle<2>::FromPosition({ localRadius * std::cos(angle), localRadius * std::sin(angle) }));
		neighbourIndices.push_back({ (i-1+n) % n, (i+1) % n });
		addParticleToGrid(i);
	}
	
	if (specificParams.attachFirstParticle && params.boundary) {
		particles.front().attached = true;
	}
}

void Surface2::addParticle() {

	// pick a random particle to insert the new particle after
	int a = int(rand01() * particles.size());
	int b = neighbourIndices[a][1];

	// insert new particle
	int c = (int)particles.size();
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

void Surface2::specificBinary(bio::BufferedBinaryFileOutput<>& data) {

	// Particle positions
	for (std::size_t i = 0; i < particles.size(); ++i) {
		bio::writeVec(data, particles[i].position);
		bio::writeSimple<std::int32_t>(data, neighbourIndices[i][1]);
	}

}
