#pragma once

#include "Surface.h"

#include <unordered_set>
#include "real.h"


/// Represents a self-avoiding tree/graph in 2D space, made up of a certain number of particles connected by lines
class Tree2 : public Surface<2, std::unordered_set<int>::const_iterator> {
	
public:
	
	struct SpecificParams {
		bool attachFirstParticle = true;
		real_t ageProbability = 0.99; // probability that a particle from which another grows will become too old to grow more
	};
	
private:
	
	// Additional parameters on top of the Surface<2>::Params
	SpecificParams specificParams;
	
	std::vector<std::unordered_set<int>> neighbourIndices; // for each particle, neighbourIndices provides all the neighbours
	std::vector<int> youngIndices; // set of particle indices that are still considered 'young' enough for new growth to occur
	
protected:
	
	inline Vec2 getNormal([[maybe_unused]] int i) override {
		return Vec2::Zero(); // @todo; can pressure force even exist in the tree?
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
		return "t2";
	}
	
public:
	
	Tree2(Params params, SpecificParams specificParams, int seed = time(nullptr));
	
	void addParticle() override;
	
	void specificJson(std::string& json) override;
	
	void specificBinary(bio::BufferedBinaryFileOutput<>& data) override;
	
};
