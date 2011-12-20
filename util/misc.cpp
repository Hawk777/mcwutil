// This warning is issued by the definitions of these macros.
#pragma GCC diagnostic ignored "-Wold-style-cast"

// This macro causes the GNU strerror_r to be defined instead of the XSI one.
#undef _GNU_SOURCE

#include "util/misc.h"
#include <string.h>
#include <sys/mman.h>

void *get_map_failed() {
	return MAP_FAILED;
}

int xsi_strerror_r(int err, char *buf, size_t buflen) {
	return strerror_r(err, buf, buflen);
}

