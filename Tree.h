#pragma once

#include "Surface.h"

#include <unordered_set>
#include "real.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/// Represents a self-avoiding tree/graph in 2D/3D space, made up of a certain number of particles connected by lines
template<int D>
class Tree : public Surface<D, std::unordered_set<int>::const_iterator> {
	
	// resolve non dependent names
	using Base = Surface<D, std::unordered_set<int>::const_iterator>;
	using Base::particles;
	using Base::addParticleToGrid;
	using Base::rand01;
	using Base::rng;
	using Base::params;
	
public:
	
	struct SpecificParams {
		bool attachFirstParticle = true;
		real_t ageProbability = 0.9; // probability that a particle from which another grows will become too old to grow more
		real_t newGrowthDistance = 0.1; // distance from old particles that new growth happens at, as a fraction of the attraction magnitude
	};
	
private:
	
	// Additional parameters on top of the Surface<2>::Params
	SpecificParams specificParams;
	
	std::vector<std::unordered_set<int>> neighbourIndices; // for each particle, neighbourIndices provides all the neighbours
	std::vector<int> youngIndices; // set of particle indices that are still considered 'young' enough for new growth to occur
	
protected:
	
	inline Vec<real_t, D> getNormal([[maybe_unused]] int i) override {
		return Vec<real_t, D>::Zero(); // @todo; can pressure force even exist in the tree?
	}
	
	inline bool areNeighbours(int i, int j) override {
		return neighbourIndices[i].find(j) != neighbourIndices[i].end();
	}
	
	inline std::unordered_set<int>::const_iterator beginNeighbours(int i) override {
		return neighbourIndices[i].begin();
	}
	
	inline std::unordered_set<int>::const_iterator endNeighbours(int i) override {
		return neighbourIndices[i].end();
	}
	
	inline real_t getRepulsion([[maybe_unused]] int i, [[maybe_unused]] int j) override {
		return 1.0; // no repulsion deltas (surface tension) in trees yet @todo
	}
	
	inline real_t getVolume() override {
		return 1.0; // @todo; what would the volume be for a tree? I don't think pressure makes any sense here either way.
	}
	
	inline std::string getTypeHint() override {
		return "t" + std::to_string(D);
	}
	
public:
	
	Tree(typename Tree<D>::Params params, SpecificParams specificParams, int seed = time(nullptr));
	
	void addParticle() override;
	
	void specificJson(std::string& json) override;
	
	void specificBinary(bio::BufferedBinaryFileOutput<>& data) override;
	
};



template<int D>
Tree<D>::Tree(typename Tree<D>::Params params, SpecificParams specificParams, int seed) : Surface<D, std::unordered_set<int>::const_iterator>(params, seed), specificParams(specificParams) {
	
    // initial state: 2 particles connected together
    particles.push_back(Particle<D>::FromPosition(Vec<real_t, D>::Zero()));
	auto secondPartPosition = Vec<real_t, D>::Zero();
	secondPartPosition.setX(params.attractionMagnitude);
    particles.push_back(Particle<D>::FromPosition(secondPartPosition));
    if (specificParams.attachFirstParticle && params.boundary) {
        particles.front().attached = true;
    } else {
        youngIndices.push_back(0);
    }
    youngIndices.push_back(1);
    neighbourIndices.push_back({ 1 });
    neighbourIndices.push_back({ 0 });
    addParticleToGrid(0);
    addParticleToGrid(1);
}

template<int D>
void Tree<D>::addParticle() {
    
    // O(1)
    
    // pick a random 'young' particle to grow from
    int youngIdx = int(rand01() * youngIndices.size());
    int a = youngIndices[youngIdx];
    
    // pick a random orientation for the young particle
    auto dir = Vec<real_t, D>::RandomUnit(rng);
	dir *= params.attractionMagnitude * specificParams.newGrowthDistance;
    
    // insert new particle
    int newIdx = (int)particles.size();
    particles.push_back(Particle<D>::FromPosition(particles[a].position + dir));
    neighbourIndices[a].insert(newIdx);
    neighbourIndices.push_back({ a });
    youngIndices.push_back(newIdx);
    addParticleToGrid(newIdx);
    
    // mark parent particle as being too old for new growth with a random probability
    if (rand01() <= specificParams.ageProbability) {
        // effectively swap the young indices for the two particles and remove the last element
        // (O(1) deletion)
        youngIndices[youngIdx] = youngIndices.back();
        youngIndices.pop_back();
    }
}

template<int D>
void Tree<D>::specificJson(std::string& json) {
    
    json += "\t'particles': [\n";
    for (std::size_t i = 0; i < particles.size(); ++i) {
		json += "\t\t{\n";
		json += "\t\t\t'position': " + particles[i].position.toString() + ",\n";
		json += "\t\t\t'velocity': " + particles[i].velocity.toString() + ",\n";
		json += "\t\t\t'acceleration': " + particles[i].acceleration.toString() + ",\n";
		json += "\t\t\t'noise': " + std::to_string(particles[i].noise) + ",\n";
        json += "\t\t\t'neighbours': [\n";
        for (auto it = neighbourIndices[i].begin(); it != neighbourIndices[i].end(); it++) {
            json += "\t\t\t\t" + std::to_string(*it) + ",\n";
        }
        json = json.substr(0, json.size() - 2) + "\n"; // remove the last ',' inserted
        json += "\t\t\t],\n";
		json += "\t\t}";
		if (i < particles.size() - 1) json += ",";
		json += "\n";
    }
    json += "\t]";
    
}

template<int D>
void Tree<D>::specificBinary(bio::BufferedBinaryFileOutput<>& data) {
    
    for (std::size_t i = 0; i < particles.size(); ++i) {
        bio::writeVec(data, particles[i].position);
        bio::writeCollection(data, neighbourIndices[i]);
    }
}
