#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <cstddef>

void *get_map_failed();
int xsi_strerror_r(int err, char *buf, std::size_t buflen);

#endif

