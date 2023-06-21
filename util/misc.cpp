// This warning is issued by the definitions of these macros.
#pragma GCC diagnostic ignored "-Wold-style-cast"

// This macro causes the GNU strerror_r to be defined instead of the XSI one.
#undef _GNU_SOURCE

#include "util/misc.h"
#include <string.h>
#include <sys/mman.h>

/**
 * \brief Gets the value of the system constant \c MAP_FAILED without provoking warnings about old C-style casts.
 *
 * \return \c MAP_FAILED.
 */
void *get_map_failed() {
	return MAP_FAILED;
}
