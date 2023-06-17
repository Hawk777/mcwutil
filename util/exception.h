#ifndef UTIL_EXCEPTION_H
#define UTIL_EXCEPTION_H

#include <stdexcept>

/**
 * \brief An exception that indicates that an attempt to fetch an error message for an error code itself failed.
 */
class ErrorMessageError : public std::runtime_error {
	public:
		/**
		 * \brief Constructs a new ErrorMessageError.
		 */
		ErrorMessageError();
};

/**
 * \brief An exception that corresponds to a system call failure.
 */
class SystemError : public std::runtime_error {
	public:
		/**
		 * \brief The error code.
		 */
		const int error_code;

		/**
		 * \brief Constructs a new SystemError for a specific error code.
		 *
		 * \param[in] call the system call that failed.
		 *
		 * \param[in] err the error code.
		 */
		SystemError(const char *call, int err);
};

/**
 * \brief An exception that corresponds to an attempt to open a file that does not exist.
 */
class FileNotFoundError : public SystemError {
	public:
		/**
		 * \brief Constructs a new FileNotFoundError.
		 */
		FileNotFoundError();
};

#endif

