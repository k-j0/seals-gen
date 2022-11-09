#pragma once

#include <string>

/// Returns the name of the machine the code is running on
std::string getMachineName();

/// Returns the current short git hash on the current branch
std::string getGitHash();

/// constexpr version of pow() (meaning it can e.g. be used inside templates)
constexpr int powConstexpr(int num, unsigned int pow) {
	return pow == 0 ? 1 : num * powConstexpr(num, pow - 1);
}
