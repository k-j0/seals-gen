
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
#include <stdio.h>

WARNING_DISABLE_OMP_PRAGMAS;
WARNING_DISABLE_STACK_SIZE;


int main(int argc, char** argv) {
	
	// Read arguments
	SurfaceBase<>* surface = nullptr;
	int iterations;
	int particleGrowth;
	bool writeJson;
    bool computeBackboneDim = false;
	std::string outFile;
	{
		Arguments args(argc, argv);
        bool sealPreset = args.read<bool>("seals", false);
		surface = SurfaceFactory::build(args, sealPreset);
        if (surface->isTree()) {
            computeBackboneDim = args.read<bool>("compute-backbone-dim", false);
        }
		iterations = args.read<int>("iter", sealPreset ? 20000 : 600);
		particleGrowth = args.read<int>("growth", 5);
		writeJson = args.read<bool>("json", false);
		std::string allArgs = "";
		for (int i = 1; i < argc; ++i) {
			allArgs += argv[i] + std::string(" ");
		}
		outFile = args.read<std::string>("out", "results/" + allArgs + "[" + getGitHash() + "].bin");
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
				surface->addParticle(real_t(t)/real_t(iterations));
			}
			#ifndef NO_UPDATE
				surface->update(real_t(t)/real_t(iterations));
				
				// recurrent outputs (console + snapshots)
				if (t % progressCheck == 0) { // 255 hits over the full generation (no matter iteration count, unless lower than 255)
					std::printf("%d %%...\r", t * 100 / iterations);
					std::fflush(stdout);
					auto millis = runtime.getMs();
					surface->toBinary(int(millis), snapshotsBinary);
					if (writeJson) {
						if (!first) {
							snapshotsJson += ",\n";
						}
						snapshotsJson += surface->toJson(int(millis));
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
				surface->update(real_t(1));
			}
		#endif
	}
	
	// Write the final snapshot
	surface->toBinary(int(totalRuntimeMs), snapshotsBinary);
	snapshotsBinary.dump();
	std::printf("Wrote results to %s", outFile.c_str());
	if (writeJson) {
		snapshotsJson += (first ? "" : ",\n") + surface->toJson(int(totalRuntimeMs));
		File::Write("results/surface.json", snapshotsJson + "\n]");
		std::printf(" and results/surface.json");
	}
	std::printf(".\n");
    
    // Compute the backbone dimension in-place if required
    if (computeBackboneDim) {
        std::printf("Computing backbone dimension...\n");
	    bio::BufferedBinaryFileOutput<> backboneDimBinary(outFile + ".d_m");
        if (surface->getDimension() == 2) {
            Tree<2>* tree = dynamic_cast<Tree<2>*>(surface);
            tree->backboneDimensionSamples(backboneDimBinary);
        } else if (surface->getDimension() == 3) {
            Tree<3>* tree = dynamic_cast<Tree<3>*>(surface);
            tree->backboneDimensionSamples(backboneDimBinary);
        }
        backboneDimBinary.dump();
        std::printf("Wrote backbone dimension samples to %s.d_m.\n", outFile.c_str());
    }
    
	delete surface;

	return 0;
}
