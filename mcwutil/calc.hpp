#ifndef CALC_H
#define CALC_H

#include <span>
#include <string_view>

namespace mcwutil {
/**
 * \brief Utility subcommands for doing useful mathematical calculations.
 */
namespace calc {
int coord(std::string_view appname, std::span<char *> args);
}
}

#endif
