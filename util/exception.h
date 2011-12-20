#ifndef UTIL_EXCEPTION_H
#define UTIL_EXCEPTION_H

#include <stdexcept>

/**
 * An exception that indicates that an attempt to fetch an error message for an error code itself failed.
 */
class ErrorMessageError : public std::runtime_error {
	public:
		/**
		 * Constructs a new ErrorMessageError.
		 */
		ErrorMessageError();
};

/**
 * An exception that corresponds to a system call failure.
 */
class SystemError : public std::runtime_error {
	public:
		/**
		 * The error code.
		 */
		const int error_code;

		/**
		 * Constructs a new SystemError for a specific error code.
		 *
		 * \param[in] call the system call that failed.
		 *
		 * \param[in] err the error code.
		 */
		SystemError(const char *call, int err);
};

/**
 * An exception that corresponds to an attempt to open a file that does not exist.
 */
class FileNotFoundError : public SystemError {
	public:
		/**
		 * Constructs a new FileNotFoundError.
		 */
		FileNotFoundError();
};

#endif

