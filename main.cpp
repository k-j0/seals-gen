
#include "Surface.h"
#include "File.h"
#include <chrono>

int main() {
	
	Surface::Params params;
	params.attractionMagnitude = 1;
	Surface surface(params, 0);

	std::chrono::high_resolution_clock clock;
	auto start = clock.now();
	for (int i = 0; i < 10000; ++i) {
		surface.addParticle();
	}
	auto end = clock.now();
	printf("Subdivided into 10k vertices: took %d ms.\n", int(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()));

	std::string json = surface.toJson();
	File::Write("results/surface.json", json);

	return 0;
}
