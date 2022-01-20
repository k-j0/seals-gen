#pragma once

#include <string>
#include <fstream>

class File {

public:

	/// Writes a string to a file
	static void Write(std::string filename, std::string contents);

};
