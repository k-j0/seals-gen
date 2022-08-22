
#include "Utils.h"

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
	char str[64];
	std::size_t _;
	wcstombs_s(&_, str, name, 64);
	WARNING_PUSH;
	WARNING_DISABLE_STRING_NUL_TERMINATED;
		result = std::string(str);
	WARNING_POP;
#else
	char name[64];
	gethostname(name, 64);
	result = std::string(name);
#endif
	isSet = true;
	return result;
}
