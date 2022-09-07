
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
#include "Arguments.h"


WARNING_DISABLE_OMP_PRAGMAS;


int main(int argc, char** argv) {
	
	printf("Starting...\n\n");
	
	// Read arguments
	int d, iterations;
	bool writeJson;
	{
		Arguments args(argc, argv);
		d = args.read<int>("d", 2);
		iterations = args.read<int>("iter", 600);
		writeJson = args.read<bool>("json", false);
	}

#ifdef _OPENMP
	#pragma omp parallel
	#pragma omp master
	{
		printf("OpenMP enabled, %d threads.\n\n", omp_get_num_threads());
	}
#endif
	
	SurfaceBase* surface = nullptr;

	if (d == 3) {
		Surface3::Params params;
		#ifdef NO_UPDATE
			params.attractionMagnitude = 1.0;
		#endif
		params.repulsionAnisotropy = Vec3(1.0, 1.0, 1.0);
		params.boundary = std::make_shared<CylinderBoundary>((real_t).15, (real_t).15);
		surface = new Surface3(params, { true, Surface3::GrowthStrategy::DELAUNAY }, 0);

	} else if (d == 2) {

		Surface2::Params params;
		params.attractionMagnitude = (real_t)0.01;
		params.damping = (real_t)0.5;
		params.dt = (real_t)0.5;
		#ifdef NO_UPDATE
			params.attractionMagnitude = (real_t)1.0;
		#endif
		params.boundary = std::make_shared<SphereBoundary<2>>((real_t)0.075, (real_t)0.5, (real_t).05, real_t(1 + 2e-4));
		surface = new Surface2(params, { true, (real_t)1.35 }, 0);

	} else {
		printf("Error: invalid dimensionality! Must select 2 or 3.");
		exit(1);
	}
	
	std::string snapshotsJson = writeJson ? "[\n" : "";
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
			int millis = int(std::chrono::duration_cast<std::chrono::milliseconds>(clock.now() - start).count());
			surface->toBinary(millis, snapshotsBinary);
			File::Write("results/surface.bin", snapshotsBinary);
			if (writeJson) {
				if (!first) {
					snapshotsJson += ",\n";
				}
				snapshotsJson += surface->toJson(millis);
				first = false;
				File::Write("results/surface.json", snapshotsJson + "\n]");
			}
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
	surface->toBinary(totalRuntime, snapshotsBinary);
	File::Write("results/surface.bin", snapshotsBinary);
	printf("Wrote results to results/surface.bin");
	if (writeJson) {
		snapshotsJson += (first ? "" : ",\n") + surface->toJson(totalRuntime);
		File::Write("results/surface.json", snapshotsJson + "\n]");
		printf(" and results/surface.json");
	}
	printf(".\n");

	delete surface;

	return 0;
}
