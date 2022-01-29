
#include "Surface.h"
#include "File.h"

int main() {
	
	Surface::Params params;
	params.attractionMagnitude = 1;
	Surface surface(params);

	std::string json = surface.toJson();
	File::Write("results/surface.json", json);

	return 0;
}
