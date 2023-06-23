#ifndef REGION_UNPACK_H
#define REGION_UNPACK_H

#include <ranges>

namespace mcwutil {
namespace Region {
int unpack(std::ranges::subrange<char **> args);
}
}

#endif
