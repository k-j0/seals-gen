
#include <chrono>
#include "Options.h"
#include "SurfaceFactory.h"
#include "File.h"
#ifdef _OPENMP
	#include <omp.h>
#endif
#include "warnings.h"
#include "Arguments.h"
#include "Runtime.h"
#include "cuda_info.h"

WARNING_DISABLE_OMP_PRAGMAS;


int main(int argc, char** argv) {
	
	// Read arguments
	SurfaceBase<>* surface = nullptr;
	int iterations;
	int particleGrowth;
	bool writeJson;
	std::string outFile;
	{
		Arguments args(argc, argv);
		surface = SurfaceFactory::build(args);
		iterations = args.read<int>("iter", 600);
		particleGrowth = args.read<int>("growth", 5);
		writeJson = args.read<bool>("json", false);
		outFile = args.read<std::string>("out", "results/surface.bin");
	}
	
#ifdef CUDA
	std::printf("%s\n\n", getCudaInfo().c_str());
#else
	std::printf("CUDA disabled.\n\n");
#endif
	
#ifdef _OPENMP
	#pragma omp parallel
	#pragma omp master
	{
		std::printf("OpenMP enabled, %d threads.\n\n", omp_get_num_threads());
	}
#else
	std::printf("OpenMP disabled.\n\n");
#endif
	
	std::printf("Starting...\n\n");
	
	std::string snapshotsJson = writeJson ? "[\n" : "";
	bio::BufferedBinaryFileOutput<> snapshotsBinary(outFile);
	bool first = true;

	// grow progressively
	long long totalRuntimeMs;
	{
		Runtime runtime(totalRuntimeMs);
		std::size_t progressCheck = iterations / 255;
		if (progressCheck == 0) progressCheck = 1;
		for (int t = 0; t < iterations; ++t) {
			
			// update surface
			if (particleGrowth > 0 && t % particleGrowth == 0) {
				surface->addParticle();
			}
			#ifndef NO_UPDATE
				surface->update();
				
				// recurrent outputs (console + snapshots)
				if (t % progressCheck == 0) { // 255 hits over the full generation (no matter iteration count, unless lower than 255)
					std::printf("%d %%...\r", t * 100 / iterations);
					std::fflush(stdout);
					auto millis = runtime.getMs();
					surface->toBinary(millis, snapshotsBinary);
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
		std::printf("100 %%  \n\n");
		
		#ifndef NO_UPDATE
			// settle (iterations without new particles)
			for (int t = 0; t < 50; ++t) {
				surface->update();
			}
		#endif
	}
	
	// Write the final snapshot
	surface->toBinary(totalRuntimeMs, snapshotsBinary);
	snapshotsBinary.dump();
	std::printf("Wrote results to %s", outFile.c_str());
	if (writeJson) {
		snapshotsJson += (first ? "" : ",\n") + surface->toJson(totalRuntimeMs);
		File::Write("results/surface.json", snapshotsJson + "\n]");
		std::printf(" and results/surface.json");
	}
	std::printf(".\n");

	delete surface;

	return 0;
}
