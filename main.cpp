
#include "Surface.h"
#include "File.h"
#include <chrono>

int main() {

	printf("Starting...\n\n");
	
	Surface::Params params;
	params.repulsionAnisotropy = Vec3(1.0, 0.1, 0.1);
	Surface surface(params, 0);

	const int iterations = 10000;
	std::string snapshotsJson = "[\n";
	bool first = true;
	std::chrono::high_resolution_clock clock;
	auto start = clock.now();

	// grow progressively
	// 10k iterations, serial, release build: ~190-240s (~3-4 mins) (of which tessellation ~0.2s (Delaunay: ~2.6s))
	for (int t = 0; t < iterations; ++t) {

		// update surface
		if (t % 5 == 0) {
			surface.addParticleDelaunay();
		}
		surface.update();

		// recurrent outputs (console + snapshots)
		if (t % (iterations / 255) == 0) { // 255 hits over the full generation (no matter iteration count)
			printf("%d %%...\r", t * 100 / iterations);
			if (!first) {
				snapshotsJson += ",\n";
			}
			snapshotsJson += surface.toJson();
			first = false;
			File::Write("results/surface.json", snapshotsJson + "\n]");
		}

	}
	printf("100 %%  \n");

	// settle (iterations without new particles)
	for (int t = 0; t < 50; ++t) {
		surface.update();
	}


	auto end = clock.now();
	printf("Total runtime: %d ms.\n", int(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()));

	// Write the final snapshot
	snapshotsJson += (first ? "" : ",\n") + surface.toJson();
	File::Write("results/surface.json", snapshotsJson + "\n]");

	return 0;
}
