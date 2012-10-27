#ifndef ZLIB_UTILS_H
#define ZLIB_UTILS_H

#include <string>
#include <vector>

namespace ZLib {
	int compress(const std::vector<std::string> &args);
	int decompress(const std::vector<std::string> &args);
	int check(const std::vector<std::string> &args);
}

#endif

