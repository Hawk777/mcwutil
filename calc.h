#ifndef CALC_H
#define CALC_H

#include <ranges>

namespace mcwutil {
/**
 * \brief Utility subcommands for doing useful mathematical calculations.
 */
namespace calc {
int coord(std::ranges::subrange<char **> args);
}
}

#endif
