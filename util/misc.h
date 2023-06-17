#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <cstddef>

/**
 * \brief Gets the value of the system constant \c MAP_FAILED without provoking warnings about old C-style casts.
 *
 * \return \c MAP_FAILED.
 */
void *get_map_failed();

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
int xsi_strerror_r(int err, char *buf, std::size_t buflen);

#endif

