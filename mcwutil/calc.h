#ifndef CALC_H
#define CALC_H

#include <span>

namespace mcwutil {
/**
 * \brief Utility subcommands for doing useful mathematical calculations.
 */
namespace calc {
int coord(std::span<char *> args);
}
}

#endif
