
#include <chrono>
#include "Options.h"
#include "Surface3.h"
#include "Surface2.h"
#include "File.h"
#include "SphereBoundary.h"
#include "CylinderBoundary.h"
#ifdef _OPENMP
	#include <omp.h>
#endif
#include "warnings.h"
#include "Arguments.h"
#include "Runtime.h"


WARNING_DISABLE_OMP_PRAGMAS;


int main(int argc, char** argv) {
	
	// Read arguments
	SurfaceBase* surface = nullptr;
	int iterations;
	int particleGrowth;
	bool writeJson;
	std::string outFile;
	{
		Arguments args(argc, argv);
		int d = args.read<int>("d", 2);
		if (d == 3) {
			Surface3::Params params;
			params.attractionMagnitude = args.read<real_t>("magnitude", .025);
			params.repulsionMagnitudeFactor = args.read<real_t>("repulsion", 2.1);
			params.damping = args.read<real_t>("damping", .15);
			params.noise = args.read<real_t>("noise", .25);
			real_t aniso = args.read<real_t>("anisotropy", 1);
			params.repulsionAnisotropy = Vec3(1.0, aniso, aniso);
			std::string boundaryType = args.read<std::string>("boundary", "cylinder");
			if (boundaryType.compare("cylinder") == 0) {
				params.boundary = std::make_shared<CylinderBoundary>(
					args.read<real_t>("boundary-radius", .15),
					args.read<real_t>("boundary-max-radius", .15),
					args.read<real_t>("boundary-extent", .05),
					args.read<real_t>("boundary-growth", 0)
				);
			} else if (boundaryType.compare("sphere") == 0) {
				params.boundary = std::make_shared<SphereBoundary<3>>(
					args.read<real_t>("boundary-radius", .15),
					args.read<real_t>("boundary-max-radius", .15),
					args.read<real_t>("boundary-extent", .05),
					args.read<real_t>("boundary-growth", 0)
				);
			}
			Surface3::SpecificParams specificParams;
			specificParams.attachFirstParticle = args.read<bool>("attach-first", false);
			std::string growthStrategy = args.read<std::string>("growth-strategy", "delaunay");
			if (growthStrategy.compare("edge") == 0) {
				specificParams.strategy = Surface3::GrowthStrategy::ON_EDGE;
			} else if (growthStrategy.compare("delaunay-aniso-edge") == 0) {
				specificParams.strategy = Surface3::GrowthStrategy::DELAUNAY_ANISO_EDGE;
			} else {
				specificParams.strategy = Surface3::GrowthStrategy::DELAUNAY;
			}
			surface = new Surface3(params, specificParams, args.read<int>("seed", 0));
		} else if (d == 2) {
			Surface2::Params params;
			params.attractionMagnitude = args.read<real_t>("magnitude", .01);
			params.repulsionMagnitudeFactor = args.read<real_t>("repulsion", 2.1);
			params.damping = args.read<real_t>("damping", .5);
			params.noise = args.read<real_t>("noise", .25);
			params.pressure = args.read<real_t>("pressure", 0);
			params.boundary = std::make_shared<SphereBoundary<2>>(
				args.read<real_t>("boundary-radius", .5),
				args.read<real_t>("boundary-max-radius", .5),
				args.read<real_t>("boundary-extent", .05),
				args.read<real_t>("boundary-growth", 0)
			);
			params.dt = 0.5;
			Surface2::SpecificParams specificParams;
			specificParams.initialParticleCount = args.read<int>("particles", 3);
			specificParams.initialNoise = args.read<real_t>("initial-noise", 0);
			specificParams.attachFirstParticle = args.read<bool>("attach-first", false);
			specificParams.surfaceTensionMultiplier = args.read<real_t>("surface-tension", 1);
			surface = new Surface2(params, specificParams);
		} else {
			std::printf("Error: invalid dimensionality %d! Must select 2 or 3.", d);
			std::exit(1);
		}
		iterations = args.read<int>("iter", 600);
		particleGrowth = args.read<int>("growth", 5);
		writeJson = args.read<bool>("json", false);
		outFile = args.read<std::string>("out", "results/surface.bin");
	}
	
	std::printf("Starting...\n\n");

#ifdef _OPENMP
	#pragma omp parallel
	#pragma omp master
	{
		std::printf("OpenMP enabled, %d threads.\n\n", omp_get_num_threads());
	}
#endif
	
	std::string snapshotsJson = writeJson ? "[\n" : "";
	std::vector<std::uint8_t> snapshotsBinary;
	bool first = true;

	// grow progressively
	long long totalRuntimeMs;
	{
		Runtime runtime(totalRuntimeMs);
		for (int t = 0; t < iterations; ++t) {
			
			// update surface
			if (particleGrowth > 0 && t % particleGrowth == 0) {
				surface->addParticle();
			}
			#ifndef NO_UPDATE
				surface->update();
				
				// recurrent outputs (console + snapshots)
				if (t % (iterations / 255) == 0) { // 255 hits over the full generation (no matter iteration count)
					std::printf("%d %%...\r", t * 100 / iterations);
					std::fflush(stdout);
					auto millis = runtime.getMs();
					surface->toBinary(millis, snapshotsBinary);
					File::Write(outFile, snapshotsBinary);
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
	File::Write(outFile, snapshotsBinary);
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
