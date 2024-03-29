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
    using Base::getNearbyParticleCount;
	
public:
	
	struct SpecificParams {
		bool attachFirstParticle = true;
		real_t ageProbability = real_t(.9); // probability that a particle from which another grows will become too old to grow more
		real_t newGrowthDistance = real_t(.1); // distance from old particles that new growth happens at, as a fraction of the attraction magnitude
		int minBranchLength = 3;
		int maxBranchLength = 10;
        int growthDensitySamples = 1; // if > 1, will pick the least locally dense out of growthDensitySamples random samples as the node to grow from
        real_t stopBranchingAfter = 1; // if < 1, will stop creating new branches after the specified normalized t, corresponding to e.g. birth for grey seals at CBL* = t* = ~0.67
        int growthMaxLeafDistance = 0; // if > 0, new branches are only allowed to grow at most growthMaxLeafDistance nodes from a leaf node
	};
    
    inline bool isTree() override { return true; }
	
private:
	
	// Additional parameters on top of the Surface<2>::Params
	SpecificParams specificParams;
    
    // Flipped to true when t first reaches stopBranchingAfter
    bool hasStoppedBranching = false;
	
	std::vector<std::unordered_set<int>> neighbourIndices; // for each particle, neighbourIndices provides all the neighbours
	std::vector<int> youngIndices; // set of particle indices that are still considered 'young' enough for new growth to occur
	
	// Given a particle idx, returns the number of particles that need to be traversed to get to a branch
	inline int distanceToBranch (int i, int comingFrom = -1) {
		
		// if the node has more than 2 neighbours, it's a branch
		if (neighbourIndices[i].size() > 2) {
			return 0;
		}
		
		// go over the neighbours (max 2 iterations), ignoring the node we're coming from, and return the branch distance of the new neighbour
		for (auto it = neighbourIndices[i].begin(); it != neighbourIndices[i].end(); it++) {
			if (*it == comingFrom) continue;
			return 1 + distanceToBranch(*it, i);
		}
		
		// if we get here, the only neighbour for the node is the node we're coming from, i.e. no more nodes to explore
		return 0;
	}
    
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
	
	inline real_t getSurfaceTension([[maybe_unused]] int i, [[maybe_unused]] int j) override {
		return 1.0; // no repulsion deltas (surface tension) in trees yet @todo
	}
	
	inline real_t getVolume() override {
		// the "volume" of the tree is just the cumsum of lengths of its branches
		real_t length = 0.0f;
		int particleCount = int(particles.size());
		#pragma omp parallel for reduction(+: length)
		for (int i = 0; i < particleCount; ++i) {
			for (auto it = neighbourIndices[i].begin(); it != neighbourIndices[i].end(); it++) {
				const int& j = *it;
				length += std::sqrt((particles[i].position - particles[j].position).lengthSqr());
			}
		}
		return length * 0.5f; // half, since we counted each branch twice
	}
	
	inline std::string getTypeHint() override {
		return "t" + std::to_string(D);
	}
	
    bool isNodeWithinLeafDistance(int node, int maxDistance, int parent = -1);
    
public:
	
	Tree(typename Tree<D>::Params params, SpecificParams specificParams, int seed = time(nullptr));
	
	void addParticle(real_t progression) override;
	
	void specificJson(std::string& json) override;
	
	void specificBinary(bio::BufferedBinaryFileOutput<>& data) override;
    
    void backboneDimensionSamples (bio::BufferedBinaryFileOutput<>& data);
    
private:
    void backboneDimensionSample (bio::BufferedBinaryFileOutput<>& data, int node, int comingFrom, int originalNode, real_t geodesicDistance);
	
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
bool Tree<D>::isNodeWithinLeafDistance(int node, int maxDistance, int parent) {
    
    if (maxDistance < 0) return false;
    
    if (neighbourIndices[node].size() == 1) return true;
    
    for (int neighbour : neighbourIndices[node]) {
        if (neighbour == parent) continue;
        if (isNodeWithinLeafDistance(neighbour, maxDistance - 1, node)) {
            return true;
        }
    }
    
    return false;
}

template<int D>
void Tree<D>::addParticle(real_t progression) {
    
    if (!hasStoppedBranching && progression > specificParams.stopBranchingAfter) {
        hasStoppedBranching = true;
        
        // remove all 'young' particles which aren't tips
        youngIndices.clear();
        for (std::size_t i = 0, sz = this->particles.size(); i < sz; ++i) {
            if (i == 0 && specificParams.attachFirstParticle) continue; // we don't consider the first node a leaf node
            if (neighbourIndices[i].size() == 1) { // leaf node
                youngIndices.push_back(i);
            }
        }
    }
    
    // O(1)
    
    // pick the least locally dense 'young' particles to potentially grow from out of n sampled
    int youngIdx = -1;
    int a = -1;
    int localDensity = -1;
    for (int i = 0; i < specificParams.growthDensitySamples;) {
        int potentialYoungIdx = int(rand01() * youngIndices.size());
        int potentialParticle = youngIndices[potentialYoungIdx];
        
        // Restrict new growth to branch tip proximity
        if (specificParams.growthMaxLeafDistance > 0 && !isNodeWithinLeafDistance(potentialParticle, specificParams.growthMaxLeafDistance)) {
            youngIndices.erase(youngIndices.begin() + potentialYoungIdx);
            continue;
        }
        
        if (specificParams.growthDensitySamples == 1) { // don't bother sampling local densities if not needed
            youngIdx = potentialYoungIdx;
            a = potentialParticle;
            break;
        }
        int potentialLocalDensity = getNearbyParticleCount(potentialParticle);
        if (localDensity < 0 || potentialLocalDensity < localDensity) {
            youngIdx = potentialYoungIdx;
            a = potentialParticle;
            localDensity = potentialLocalDensity;
        }
        ++i;
    }
    assert(a >= 0);
    
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
    
    // mark parent particle as being too old for new growth with a random probability if the branch is within minBranchLength and maxBranchLength
	// or if the branch is too small to branch off
	// on the other hand if the branch is too long, force the node to accept new growth sometime in the future
	int branchLength = distanceToBranch(newIdx);
	bool allowGrowth =
        progression > specificParams.stopBranchingAfter ?
            false :
		branchLength <= specificParams.minBranchLength ?
			false :
		branchLength >= specificParams.maxBranchLength ?
			true :
		rand01() >= specificParams.ageProbability ?
			true :
			false;
    if (!allowGrowth) {
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
		json += "\t\t\t'noise': 0,\n";
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
	bio::writeCollection(data, youngIndices);
}


// Only consider nodes that are towards the origin when taking backbone dim samples; others are too close to the boundary for comfort
#define CONSIDER_NODE_D_M(i) (particles[i].position.lengthSqr() < real_t(0.5*0.5))

template<int D>
void Tree<D>::backboneDimensionSamples (bio::BufferedBinaryFileOutput<>& data) {
    for (std::size_t i = 0, sz = particles.size(); i < sz; ++i) {
        if (CONSIDER_NODE_D_M(i)) {
            backboneDimensionSample(data, i, -1, i, real_t(0));
        }
    }
}

template<int D>
void Tree<D>::backboneDimensionSample (bio::BufferedBinaryFileOutput<>& data, int node, int comingFrom, int originalNode, real_t geodesicDistance) {
    for (auto it = beginNeighbours(node), end = endNeighbours(node); it != end; it++) {
        int neighbour = *it;
        if (neighbour == comingFrom) continue;
        geodesicDistance += std::sqrt((particles[neighbour].position - particles[node].position).lengthSqr());
        
        if (CONSIDER_NODE_D_M(neighbour)) {
            real_t euclideanDistance = std::sqrt((particles[neighbour].position - particles[originalNode].position).lengthSqr());
            bio::writeSimple<real_t>(data, euclideanDistance);
            bio::writeSimple<real_t>(data, geodesicDistance);
        }
        
        backboneDimensionSample(data, neighbour, node, originalNode, geodesicDistance);
    }
}

#undef CONSIDER_NODE_D_M
