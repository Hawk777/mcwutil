#ifndef REGION_REGION_H
#define REGION_REGION_H

#include <span>

namespace mcwutil {
/**
 * \brief Symbols related to the MCRegion/Anvil format.
 */
namespace region {
int pack(std::span<char *> args);
int unpack(std::span<char *> args);
}
}

#endif
