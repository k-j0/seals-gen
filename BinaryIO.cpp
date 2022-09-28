
#include "BinaryIO.h"

void bio::writeString(std::vector<std::uint8_t>& data, const std::string& val) {
	for (const char& c : val) {
		writeSimple<char>(data, c);
	}
	writeSimple<char>(data, '\0');
}

std::string bio::readString(const std::vector<std::uint8_t>& data, std::size_t& at) {
	std::string s = "";
	assert(at < data.size());
	while (at < data.size()) {
		char c = readSimple<char>(data, at);
		if (c == '\0') break;
		else s += c;
	}
	return s;
}
