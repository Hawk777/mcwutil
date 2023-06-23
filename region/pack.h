#ifndef REGION_PACK_H
#define REGION_PACK_H

#include <ranges>

namespace mcwutil {
namespace Region {
int pack(std::ranges::subrange<char **> args);
}
}

#endif
