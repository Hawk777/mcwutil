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

ErrorMessageError::ErrorMessageError() : std::runtime_error("Error fetching error message") {
}

SystemError::SystemError(const char *call, int err) : std::runtime_error(get_error_string(call, err)), error_code(err) {
}

FileNotFoundError::FileNotFoundError() : SystemError("open", ENOENT) {
}

