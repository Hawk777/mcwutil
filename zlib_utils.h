#ifndef ZLIB_UTILS_H
#define ZLIB_UTILS_H

#include <ranges>

namespace mcwutil {
/**
 * \brief Symbols related to the ZLib compression format.
 */
namespace zlib {
int compress(std::ranges::subrange<char **> args);
int decompress(std::ranges::subrange<char **> args);
int check(std::ranges::subrange<char **> args);
}
}

#endif
