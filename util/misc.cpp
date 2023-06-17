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

/**
 * \brief Executes the XSI version of the \c strerror_r function.
 *
 * \param[in] err the error code to translate.
 *
 * \param[in] buf the buffer in which to store the message.
 *
 * \param[in] buflen the length of the buffer.
 *
 * \return 0 on success, or -1 on failure.
 */
int xsi_strerror_r(int err, char *buf, size_t buflen) {
	return strerror_r(err, buf, buflen);
}

