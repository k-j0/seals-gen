
#include <chrono>
#include "Options.h"
#include "Surface.h"
#include "File.h"
#include "CylinderBoundary.h"
#ifdef _OPENMP
	#include <omp.h>
#endif

int main() {

	printf("Starting...\n\n");

#ifdef _OPENMP
	#pragma omp parallel
	#pragma omp master
	{
		printf("OpenMP enabled, %d threads.\n\n", omp_get_num_threads());
	}
#endif
	
	Surface::Params params;
#ifdef NO_UPDATE
	params.attractionMagnitude = 1.0;
#endif
	params.repulsionAnisotropy = Vec3(0.1, 0.1, 1.0);
	params.boundary = std::shared_ptr<BoundaryCondition>(new CylinderBoundary(.4));
	Surface surface(params, 0);

	const int iterations = 10000;
	std::string snapshotsJson = "[\n";
	bool first = true;
	std::chrono::high_resolution_clock clock;
	auto start = clock.now();

	// grow progressively
	// 10k iterations, serial, release build: ~190-240s (~3-4 mins) (of which tessellation ~0.2s (Delaunay: ~2.6s))
	// 10k iterations, omp, release build: ~60s
	for (int t = 0; t < iterations; ++t) {

		// update surface
		if (t % 5 == 0) {
			surface.addParticleDelaunay();
		}
#ifndef NO_UPDATE
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
#endif
	}
	printf("100 %%  \n");

#ifndef NO_UPDATE
	// settle (iterations without new particles)
	for (int t = 0; t < 50; ++t) {
		surface.update();
	}
#endif

	auto end = clock.now();
	printf("Total runtime: %d ms.\n", int(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()));

	// Write the final snapshot
	snapshotsJson += (first ? "" : ",\n") + surface.toJson();
	File::Write("results/surface.json", snapshotsJson + "\n]");

	return 0;
}
