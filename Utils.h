#pragma once

#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/// Returns the name of the machine the code is running on
std::string getMachineName() {
	static std::string result;
	static bool isSet = false;
	if (isSet) return result;
#ifdef _WIN32
	DWORD size = 64;
	TCHAR name[64];
	GetComputerName(name, &size);
	char str[64];
	std::size_t _;
	wcstombs_s(&_, str, name, 64);
	result = std::string(str);
#else
	char name[64];
	gethostname(name, 64);
	result = std::string(name);
#endif
	isSet = true;
	return result;
}
