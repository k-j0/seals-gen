
#include "Surface.h"
#include "File.h"
#include <chrono>

int main() {

	printf("Starting...\n\n");
	
	Surface::Params params;
	Surface surface(params, 0);

	std::chrono::high_resolution_clock clock;
	auto start = clock.now();
	
	/*for (int i = 0; i < 10000 / 5; ++i) {
		surface.addParticle();
	}*/

	// grow progressively
	// 10k iterations, serial, release build: ~240s (~4 mins) (of which tessellation ~.03s)
	const int iterations = 10000;
	for (int t = 0; t < iterations; ++t) {
		if (t % 5 == 0) {
			surface.addParticle();
		}
		if (t % (iterations / 100) == 0) {
			printf("%d %%...\r", t * 100 / iterations);
		}
		surface.update();
	}
	printf("100 %%  \n");

	// settle
	for (int t = 0; t < 100; ++t) {
		surface.update();
	}


	auto end = clock.now();
	printf("Total runtime: %d ms.\n", int(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()));

	std::string json = surface.toJson();
	File::Write("results/surface.json", json);

	return 0;
}
