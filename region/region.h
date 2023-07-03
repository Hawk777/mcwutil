#ifndef REGION_REGION_H
#define REGION_REGION_H

#include <ranges>

namespace mcwutil {
/**
 * \brief Symbols related to the MCRegion/Anvil format.
 */
namespace region {
int pack(std::ranges::subrange<char **> args);
int unpack(std::ranges::subrange<char **> args);
}
}

#endif
