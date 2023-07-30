#ifndef ZLIB_UTILS_H
#define ZLIB_UTILS_H

#include <span>

namespace mcwutil {
/**
 * \brief Symbols related to the ZLib compression format.
 */
namespace zlib {
int compress(std::span<char *> args);
int decompress(std::span<char *> args);
int check(std::span<char *> args);
}
}

#endif
