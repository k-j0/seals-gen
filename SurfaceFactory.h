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
            params.attractionMagnitude = args.read<real_t>("magnitude", .025);
            params.repulsionMagnitudeFactor = args.read<real_t>("repulsion", 2.1);
            params.damping = args.read<real_t>("damping", .15);
            params.noise = args.read<real_t>("noise", .25);
            real_t aniso = args.read<real_t>("anisotropy", 1);
            params.repulsionAnisotropy = Vec3(aniso, aniso, 1.0);
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
            params.dt = args.read<real_t>("dt", 0.15);
            return params;
        }
        
        template<typename T = Surface2>
        typename T::Params buildSurface2Params (Arguments& args) {
            typename T::Params params;
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
            params.dt = args.read<real_t>("dt", 0.5);
            return params;
        }
        
        template<int D>
        typename Tree<D>::SpecificParams buildTreeSParams (Arguments& args) {
            typename Tree<D>::SpecificParams specificParams;
            specificParams.attachFirstParticle = args.read<bool>("attach-first", false);
            specificParams.ageProbability = args.read<real_t>("age-prob", 0.9);
            specificParams.newGrowthDistance = args.read<real_t>("growth-distance", 0.1);
            specificParams.minBranchLength = args.read<int>("min-branch-len", 3);
            specificParams.maxBranchLength = args.read<int>("max-branch-len", 10);
            return specificParams;
        }
        
    }
    
    
    // From the command-line arguments, instantiates the relevant surface model on the heap
    SurfaceBase<>* build (Arguments& args) {
        
        SurfaceBase<>* surface = nullptr;
        
        // Read base dimensionality (2 or 3) and type (surf or tree)
        int d = args.read<int>("d", 2);
        bool tree = args.read<bool>("tree", false);
        
        int seed = args.read<int>("seed", 0);
        
        // Depending on dimensionality and type, build up the model to use
        if (d == 3) {
            if (tree) {
                surface = new Tree<3>(buildSurface3Params<Tree<3>>(args), buildTreeSParams<3>(args), seed);
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
                surface = new Surface3(params, specificParams, seed);
            }
        } else if (d == 2) {
            if (tree) {
                surface = new Tree<2>(buildSurface2Params<Tree<2>>(args), buildTreeSParams<2>(args), seed);
            } else {
                auto params = buildSurface2Params<>(args);
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
