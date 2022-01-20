#include "File.h"

void File::Write(std::string filename, std::string contents) {
	
	std::ofstream file(filename);
	file << contents;
	file.close();

}
