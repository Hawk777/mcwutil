#ifndef ZLIB_UTILS_H
#define ZLIB_UTILS_H

#include <span>
#include <string_view>

namespace mcwutil {
/**
 * \brief Symbols related to the ZLib compression format.
 */
namespace zlib {
int compress(std::string_view appname, std::span<char *> args);
int decompress(std::string_view appname, std::span<char *> args);
int check(std::string_view appname, std::span<char *> args);
}
}

#endif
