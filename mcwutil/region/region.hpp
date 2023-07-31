#ifndef REGION_REGION_H
#define REGION_REGION_H

#include <span>
#include <string_view>

namespace mcwutil {
/**
 * \brief Symbols related to the MCRegion/Anvil format.
 */
namespace region {
int pack(std::string_view appname, std::span<char *> args);
int unpack(std::string_view appname, std::span<char *> args);
}
}

#endif
