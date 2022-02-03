
#include "Surface.h"
#include "File.h"
#include <chrono>

int main() {

	printf("Starting...\n\n");
	
	Surface::Params params;
	Surface surface(params, 0);

	std::chrono::high_resolution_clock clock;
	auto start = clock.now();

	// grow progressively
	// 10k iterations, serial, release build: ~190-240s (~3-4 mins) (of which tessellation ~.03s)
	const int iterations = 5000;
	std::string snapshotsJson = "[\n";
	for (int t = 0; t < iterations; ++t) {

		// update surface
		if (t % 5 == 0) {
			surface.addParticle();
		}
		surface.update();

		// recurrent outputs (console + snapshots)
		if (t % (iterations / 255) == 0) { // 255 hits over the full generation (no matter iteration count)
			printf("%d %%...\r", t * 100 / iterations);
			snapshotsJson += surface.toJson() + ",\n";
		}

	}
	printf("100 %%  \n");

	// settle (iterations without new particles)
	for (int t = 0; t < 50; ++t) {
		surface.update();
	}


	auto end = clock.now();
	printf("Total runtime: %d ms.\n", int(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()));

	snapshotsJson += surface.toJson() + "\n]";
	File::Write("results/surface.json", snapshotsJson);

	return 0;
}
