#include "Surface2.h"

#define _USE_MATH_DEFINES
#include <math.h>

Surface2::Surface2(Params params, int seed) : Surface<2>(params, seed) {
	
	// build initial equilateral triangle with side length = attraction magnitude
	double radius = params.attractionMagnitude;
	particles.push_back(Particle<2>::FromPosition(Vec2(radius, 0.0)));
	particles.push_back(Particle<2>::FromPosition(Vec2(std::cos(2 * M_PI / 3) * radius, std::sin(2 * M_PI / 3) * radius)));
	particles.push_back(Particle<2>::FromPosition(Vec2(std::cos(4 * M_PI / 3) * radius, std::sin(4 * M_PI / 3) * radius)));
	
	// init edges
	nextNeighbourIndices.push_back(1);
	nextNeighbourIndices.push_back(2);
	nextNeighbourIndices.push_back(0);
	previousNeighbourIndices.push_back(2);
	previousNeighbourIndices.push_back(0);
	previousNeighbourIndices.push_back(1);

}

void Surface2::addParticle() {

	// pick a random particle to insert the new particle after
	int a = int(rand01() * particles.size());
	int b = nextNeighbourIndices[a];

	// insert new particle
	int c = particles.size();
	particles.push_back(Particle<2>::FromPosition((particles[a].position + particles[b].position) * 0.5));
	nextNeighbourIndices.push_back(b);
	previousNeighbourIndices.push_back(a);

	// update neighbour indices
	nextNeighbourIndices[a] = c;
	previousNeighbourIndices[b] = c;
}

void Surface2::update() {

	// @todo: extract specific behaviour into virtual Surface<D> function and group common behaviour with 3D version

	int numParticles = (int)particles.size();

	#pragma omp parallel for
	for (int i = 0; i < numParticles; ++i) {

		// dampen acceleration
		particles[i].acceleration.multiply(params.damping * params.damping);

		// boundary restriction force
		if (params.boundary) {
			particles[i].acceleration.add(params.boundary->force(particles[i].position));
		}

		// iterate over non-neighbour particles
		for (size_t j = 0; j < particles.size(); ++j) {
			if (i == j || nextNeighbourIndices[i] == j || nextNeighbourIndices[j] == i) continue; // same particle, or nearest neighbours

			// repel if close enough
			Vec2 towards = particles[j].position - particles[i].position;
			double noise = (1 + particles[i].noise * params.noise);
			double repulsionLen = params.attractionMagnitude * params.repulsionMagnitudeFactor;
			double d2 = towards.lengthSqr() * noise * noise; // d^2 to skip sqrt most of the time
			if (d2 < repulsionLen * repulsionLen) {
				towards.normalize();
				towards.multiply(sqrt(d2) - repulsionLen);
				particles[i].acceleration.add(towards <hadamard> params.repulsionAnisotropy);
			}
		}

		// iterate over neighbour particles
		for (const int& neighbour : { nextNeighbourIndices[i], previousNeighbourIndices[i] }) {

			// attract if far, repel if too close
			Vec2 towards = particles[neighbour].position - particles[i].position;
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

void Surface2::specificJson(std::string& json) {

	json += "\t'particles': [\n";
	for (std::size_t i = 0; i < particles.size(); ++i) {
		json += "\t\t{\n";
		json += "\t\t\t'position': " + particles[i].position.toString() + ",\n";
		json += "\t\t\t'velocity': " + particles[i].velocity.toString() + ",\n";
		json += "\t\t\t'acceleration': " + particles[i].acceleration.toString() + ",\n";
		json += "\t\t\t'noise': " + std::to_string(particles[i].noise) + ",\n";
		json += "\t\t\t'next': " + std::to_string(nextNeighbourIndices[i]) + ",\n";
		json += "\t\t\t'previous': " + std::to_string(previousNeighbourIndices[i]) + "\n";
		json += "\t\t}";
		if (i < particles.size() - 1) json += ",";
		json += "\n";
	}
	json += "\t]";
}

void Surface2::specificBinary(std::vector<uint8_t>& data) {}
