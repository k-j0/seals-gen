
#include "Tree2.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


Tree2::Tree2(Params params, SpecificParams specificParams, int seed) : Surface<2, std::unordered_set<int>::const_iterator>(params, seed), specificParams(specificParams) {
    
    // initial state: 2 particles connected together
    particles.push_back(Particle<2>::FromPosition(Vec2::Zero()));
    particles.push_back(Particle<2>::FromPosition({ params.attractionMagnitude, 0 }));
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

void Tree2::addParticle() {
    
    // O(1)
    
    // pick a random 'young' particle to grow from
    int youngIdx = int(rand01() * youngIndices.size());
    int a = youngIndices[youngIdx];
    
    // pick a random orientation for the young particle
    real_t theta = rand01() * 2 * M_PI;
    Vec2 dir = { std::cos(theta), std::sin(theta) };
    
    // insert new particle
    int newIdx = (int)particles.size();
    particles.push_back(Particle<2>::FromPosition(particles[a].position + dir * params.attractionMagnitude));
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

void Tree2::specificJson(std::string& json) {
    
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

void Tree2::specificBinary(bio::BufferedBinaryFileOutput<>& data) {
    
    for (std::size_t i = 0; i < particles.size(); ++i) {
        bio::writeVec(data, particles[i].position);
        bio::writeCollection(data, neighbourIndices[i]);
    }
}
