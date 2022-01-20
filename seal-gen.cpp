
#include "Surface.h"
#include "File.h"

int main() {
	
	Surface surface(50);
	std::string json = surface.toJson();

	File::Write("results/surface.json", json);

	return 0;
}
