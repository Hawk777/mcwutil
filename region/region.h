#ifndef REGION_REGION_H
#define REGION_REGION_H

#include <ranges>

namespace mcwutil {
namespace region {
int pack(std::ranges::subrange<char **> args);
int unpack(std::ranges::subrange<char **> args);
}
}

#endif
