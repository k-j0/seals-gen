
#include <chrono>
#include "Options.h"
#include "Surface3.h"
#include "Surface2.h"
#include "File.h"
#include "CylinderBoundary.h"
#ifdef _OPENMP
	#include <omp.h>
#endif
#include "warnings.h"


WARNING_DISABLE_OMP_PRAGMAS;


int main() {

	printf("Starting...\n\n");

#ifdef _OPENMP
	#pragma omp parallel
	#pragma omp master
	{
		printf("OpenMP enabled, %d threads.\n\n", omp_get_num_threads());
	}
#endif

	int d = 2;
	SurfaceBase* surface = nullptr;
	
	if (d == 3) {
		Surface3::Params params;
		#ifdef NO_UPDATE
			params.attractionMagnitude = 1.0;
		#endif
		params.repulsionAnisotropy = Vec3(1.0, 1.0, 1.0);
		params.boundary = std::shared_ptr<BoundaryCondition<3>>(new CylinderBoundary(.15));
		surface = new Surface3(params, { Surface3::GrowthStrategy::DELAUNAY }, 0);

	} else if (d == 2) {

		Surface2::Params params;
		params.attractionMagnitude = 0.01;
		params.damping = 0.5;
		params.dt = 0.5;
		#ifdef NO_UPDATE
			params.attractionMagnitude = 1.0;
		#endif
		params.boundary = std::shared_ptr<BoundaryCondition<2>>(new SphereBoundary<2>(0.5));
		surface = new Surface2(params, { 1.15 }, 0);

	} else {
		printf("Error: invalid dimensionality! Must select 2 or 3.");
		exit(1);
	}

	const int iterations = 600;
	std::string snapshotsJson = "[\n";
	std::vector<uint8_t> snapshotsBinary;
	bool first = true;
	std::chrono::system_clock clock;
	auto start = clock.now();

	// grow progressively
	// 10k iterations, serial, release build: ~190-240s (~3-4 mins) (of which tessellation ~0.2s (Delaunay: ~2.6s))
	// 10k iterations, omp, release build: ~60s
	for (int t = 0; t < iterations; ++t) {

		// update surface
		if (t % 5 == 0) {
			surface->addParticle();
		}
#ifndef NO_UPDATE
		surface->update();

		// recurrent outputs (console + snapshots)
		if (t % (iterations / 255) == 0) { // 255 hits over the full generation (no matter iteration count)
			printf("%d %%...\r", t * 100 / iterations);
			fflush(stdout);
			if (!first) {
				snapshotsJson += ",\n";
			}
			int millis = int(std::chrono::duration_cast<std::chrono::milliseconds>(clock.now() - start).count());
			snapshotsJson += surface->toJson(millis);
			surface->toBinary(millis, snapshotsBinary);
			first = false;
			File::Write("results/surface.json", snapshotsJson + "\n]");
			File::Write("results/surface.bin", snapshotsBinary);
		}
#endif
	}
	printf("100 %%  \n");

#ifndef NO_UPDATE
	// settle (iterations without new particles)
	for (int t = 0; t < 50; ++t) {
		surface->update();
	}
#endif

	auto end = clock.now();
	int totalRuntime = int(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
	printf("Total runtime: %d ms.\n", totalRuntime);

	// Write the final snapshot
	snapshotsJson += (first ? "" : ",\n") + surface->toJson(totalRuntime);
	surface->toBinary(totalRuntime, snapshotsBinary);
	File::Write("results/surface.json", snapshotsJson + "\n]");
	File::Write("results/surface.bin", snapshotsBinary);
	printf("Wrote results to results/surface.json and results/surface.bin.\n");

	delete surface;

	return 0;
}
