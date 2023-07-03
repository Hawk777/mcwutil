#ifndef NBT_NBT_H
#define NBT_NBT_H

#include <span>

namespace mcwutil {
/**
 * \brief Symbols related to the Named Binary Tag (NBT) format.
 */
namespace nbt {
int to_xml(std::span<char *> args);
int from_xml(std::span<char *> args);
int block_substitute(std::span<char *> args);
int patch_barray(std::span<char *> args);
}
}

#endif
