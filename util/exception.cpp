#include "util/exception.h"
#include "util/misc.h"
#include <cerrno>
#include <string>
#include <string.h>
#include <vector>

namespace {
	std::string get_error_string(const std::string &call, int err) {
		std::vector<char> buffer(32);
		while (xsi_strerror_r(err, &buffer[0], buffer.size()) < 0) {
			int foo = errno;
			errno = foo;
			if (errno == ERANGE) {
				buffer.resize(buffer.size() * 2);
			} else {
				throw ErrorMessageError();
			}
		}
		return call + ": " + std::string(&buffer[0]);
	}
}

/**
 * \brief Constructs a new ErrorMessageError.
 */
ErrorMessageError::ErrorMessageError() : std::runtime_error("Error fetching error message") {
}

/**
 * \brief Constructs a new SystemError for a specific error code.
 *
 * \param[in] call the system call that failed.
 *
 * \param[in] err the error code.
 */
SystemError::SystemError(const char *call, int err) : std::runtime_error(get_error_string(call, err)), error_code(err) {
}

/**
 * \brief Constructs a new FileNotFoundError.
 */
FileNotFoundError::FileNotFoundError() : SystemError("open", ENOENT) {
}

