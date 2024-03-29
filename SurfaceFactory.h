#pragma once

#include "Surface2.h"
#include "Surface3.h"
#include "Tree.h"
#include "SphereBoundary.h"
#include "CylinderBoundary.h"
#include "Arguments.h"

namespace SurfaceFactory {
    
    // Common parameters extracted from the build() method
    namespace {
        
        template<typename T = Surface3>
        typename T::Params buildSurface3Params (Arguments& args) {
            typename T::Params params;
            params.attractionMagnitude = args.read<real_t>("magnitude", real_t(.025));
            params.repulsionMagnitudeFactor = args.read<real_t>("repulsion", real_t(2.1));
            if (args.read<bool>("overdamped", false)) {
                params.damping = 0;
            } else {
                params.damping = args.read<real_t>("damping", real_t(.15));
            }
            params.pressure = args.read<real_t>("pressure", real_t(0));
            params.targetVolume = args.read<real_t>("target-volume", real_t(-1));
            params.finalTargetVolume = args.read<real_t>("final-target-volume", real_t(1));
            real_t aniso = args.read<real_t>("anisotropy", real_t(1));
            params.repulsionAnisotropy = Vec3(aniso, aniso, real_t(1.0));
            params.repelByMaxNeighbourDist = args.read<bool>("rep-max-neighbour");
            if (!params.repelByMaxNeighbourDist) { // when using the new repulsion strategy, adaptive repulsion is turned off
                params.adaptiveRepulsion = args.read<real_t>("adaptive-repulsion", real_t(0));
            }
            params.rigidity = args.read<real_t>("rigidity", real_t(0));
            std::string boundaryType = args.read<std::string>("boundary", "cylinder");
            if (boundaryType.compare("cylinder") == 0) {
                params.boundary = std::make_shared<CylinderBoundary>(
                    args.read<real_t>("boundary-radius", real_t(.15)),
                    args.read<real_t>("boundary-max-radius", real_t(.15)),
                    args.read<real_t>("boundary-extent", real_t(.05)),
                    args.read<real_t>("boundary-growth", real_t(0))
                );
            } else if (boundaryType.compare("sphere") == 0) {
                params.boundary = std::make_shared<SphereBoundary<3>>(
                    args.read<real_t>("boundary-radius", real_t(.15)),
                    args.read<real_t>("boundary-max-radius", real_t(.15)),
                    args.read<real_t>("boundary-extent", real_t(.05)),
                    args.read<real_t>("boundary-growth", real_t(0)),
                    args.read<real_t>("boundary-target-density", real_t(0)),
                    args.read<bool>("boundary-offset", false)
                );
            }
            params.dt = args.read<real_t>("dt", real_t(.15));
            return params;
        }
        
        template<typename T = Surface2>
        typename T::Params buildSurface2Params (Arguments& args, bool sealPreset) {
            typename T::Params params;
            params.attractionMagnitude = args.read<real_t>("magnitude", real_t(.01));
            params.repulsionMagnitudeFactor = args.read<real_t>("repulsion", real_t(2.1));
            if (args.read<bool>("overdamped", sealPreset)) {
                params.damping = 0;
            } else {
                params.damping = args.read<real_t>("damping", real_t(.5));
            }
            params.pressure = args.read<real_t>("pressure", real_t(0));
            params.targetVolume = args.read<real_t>("target-volume", real_t(-1));
            params.finalTargetVolume = args.read<real_t>("final-target-volume", real_t(1));
            params.repelByMaxNeighbourDist = args.read<bool>("rep-max-neighbour");
            if (!params.repelByMaxNeighbourDist) { // when using the new repulsion strategy, adaptive repulsion is turned off
                params.adaptiveRepulsion = args.read<real_t>("adaptive-repulsion", real_t(sealPreset ? .15 : 0));
            }
            params.rigidity = args.read<real_t>("rigidity", real_t(sealPreset ? .00025 : 0));
            params.boundary = args.read<std::string>("boundary", "circle").compare("circle") == 0 ? std::make_shared<SphereBoundary<2>>(
                args.read<real_t>("boundary-radius", real_t(sealPreset ? .05 : .5)),
                args.read<real_t>("boundary-max-radius", real_t(.5)),
                args.read<real_t>("boundary-extent", real_t(.05)),
                args.read<real_t>("boundary-growth", real_t(0)),
                args.read<real_t>("boundary-target-density", real_t(sealPreset ? 50 : 0)),
                args.read<bool>("boundary-offset", sealPreset)
            ) : nullptr;
            params.dt = args.read<real_t>("dt", real_t(0.5));
            return params;
        }
        
        template<int D>
        typename Tree<D>::SpecificParams buildTreeSParams (Arguments& args, bool sealPreset) {
            typename Tree<D>::SpecificParams specificParams;
            specificParams.attachFirstParticle = args.read<bool>("attach-first", sealPreset);
            specificParams.ageProbability = args.read<real_t>("age-prob", real_t(.9));
            specificParams.newGrowthDistance = args.read<real_t>("growth-distance", real_t(.1));
            specificParams.minBranchLength = args.read<int>("min-branch-len", sealPreset ? 1 : 3);
            specificParams.maxBranchLength = args.read<int>("max-branch-len", sealPreset ? 3 : 10);
            specificParams.growthDensitySamples = args.read<int>("growth-density-samples", sealPreset ? 15 : 1);
            specificParams.stopBranchingAfter = args.read<real_t>("stop-branching-after", real_t(1));
            specificParams.growthMaxLeafDistance = args.read<int>("max-leaf-distance", 0);
            return specificParams;
        }
        
    }
    
    
    // From the command-line arguments, instantiates the relevant surface model on the heap
    SurfaceBase<>* build (Arguments& args, bool sealPreset) {
        
        SurfaceBase<>* surface = nullptr;
        
        // Read base dimensionality (2 or 3) and type (surf or tree)
        int d = args.read<int>("d", 2);
        bool tree = args.read<bool>("tree", sealPreset);
        
        int seed = args.read<int>("seed", 0);
        
        // Depending on dimensionality and type, build up the model to use
        if (d == 3) {
            if (tree) {
                surface = new Tree<3>(buildSurface3Params<Tree<3>>(args), buildTreeSParams<3>(args, false), seed);
            } else {
                auto params = buildSurface3Params<>(args);
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
                specificParams.surfaceTensionMultiplier = args.read<real_t>("surface-tension", 1);
                surface = new Surface3(params, specificParams, seed);
            }
        } else if (d == 2) {
            if (tree) {
                surface = new Tree<2>(buildSurface2Params<Tree<2>>(args, sealPreset), buildTreeSParams<2>(args, sealPreset), seed);
            } else {
                auto params = buildSurface2Params<>(args, false);
                Surface2::SpecificParams specificParams;
                specificParams.initialParticleCount = args.read<int>("particles", 3);
                specificParams.initialNoise = args.read<real_t>("initial-noise", 0);
                specificParams.attachFirstParticle = args.read<bool>("attach-first", false);
                specificParams.surfaceTensionMultiplier = args.read<real_t>("surface-tension", 1);
                surface = new Surface2(params, specificParams, seed);
            }
        } else {
            std::printf("Error: invalid dimensionality %d! Must select 2 or 3.", d);
            std::exit(1);
        }
        
        return surface;
    }
    
}
