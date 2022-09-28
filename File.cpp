#include "File.h"

void File::Write(std::string filename, std::string contents) {
	
	std::ofstream file(filename);
	file << contents;
	file.close();

}

void File::Write(std::string filename, std::vector<std::uint8_t> contents) {
	
	std::ofstream file(filename, std::ios::binary);
	file.write(reinterpret_cast<const char*>(&contents[0]), contents.size());
	file.close();

}
