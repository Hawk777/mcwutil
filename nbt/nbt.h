#ifndef NBT_NBT_H
#define NBT_NBT_H

#include <ranges>

namespace mcwutil {
/**
 * \brief Symbols related to the Named Binary Tag (NBT) format.
 */
namespace nbt {
int to_xml(std::ranges::subrange<char **> args);
int from_xml(std::ranges::subrange<char **> args);
int block_substitute(std::ranges::subrange<char **> args);
int patch_barray(std::ranges::subrange<char **> args);
}
}

#endif
