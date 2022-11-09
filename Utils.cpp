
#include "Utils.h"

#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#elif defined(__CYGWIN__)
#include <winsock.h>
#else
#include <unistd.h>
#endif

#include "warnings.h"


std::string getMachineName() {
	static std::string result;
	static bool isSet = false;
	if (isSet) return result;
#ifdef _WIN32
	DWORD size = 64;
	TCHAR name[64];
	GetComputerName(name, &size);
	#ifndef UNICODE
		result = std::string(name);
	#else
		char str[64];
		std::size_t _;
		wcstombs_s(&_, str, name, 64);
		WARNING_PUSH;
		WARNING_DISABLE_STRING_NUL_TERMINATED;
			result = std::string(str);
		WARNING_POP;
	#endif
#else
	char name[64];
	gethostname(name, 64);
	result = std::string(name);
#endif
	isSet = true;
	return result;
}


std::string getGitHash () {
	// Reads .git/HEAD file to find the ref, then reads the ref file to get the hash
	
	std::ifstream headFile(".git/HEAD");
	std::stringstream headBuffer;
	headBuffer << headFile.rdbuf();
	std::string head = headBuffer.str();
	head = head.substr(5, head.size()-6);// "ref: ".length
	
	std::ifstream refFile(".git/" + head);
	std::stringstream refBuffer;
	refBuffer << refFile.rdbuf();
	
	return refBuffer.str().substr(0, 8); // first 8 characters only for the short hash
}
