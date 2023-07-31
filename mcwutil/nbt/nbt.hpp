#ifndef NBT_NBT_H
#define NBT_NBT_H

#include <span>
#include <string_view>

namespace mcwutil {
/**
 * \brief Symbols related to the Named Binary Tag (NBT) format.
 */
namespace nbt {
int to_xml(std::string_view appname, std::span<char *> args);
int from_xml(std::string_view appname, std::span<char *> args);
int block_substitute(std::string_view appname, std::span<char *> args);
int patch_barray(std::string_view appname, std::span<char *> args);
}
}

#endif
