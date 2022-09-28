#pragma once

#include <string>
#include <vector>
#include <fstream>

class File {

public:

	/// Writes a string to a file
	static void Write(std::string filename, std::string contents);

	/// Writes a binary string to a file
	static void Write(std::string filename, std::vector<std::uint8_t> contents);

};
