#include "util/fd.h"
#include "util/exception.h"
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <glibmm/miscutils.h>
#include <sys/socket.h>
#include <sys/types.h>

/**
 * \brief Constructs a new FileDescriptor with a descriptor.
 *
 * \param[in] fd the existing file descriptor, of which ownership is taken.
 *
 * \return a new FileDescriptor owning \p fd.
 */
FileDescriptor FileDescriptor::create_from_fd(int fd) {
	return FileDescriptor(fd);
}

/**
 * \brief Constructs a new FileDescriptor by calling \c open(2).
 *
 * \param[in] file the name of the file to open or create.
 *
 * \param[in] flags the file flags to use as per \c open(2).
 *
 * \param[in] mode the permissions to create a new file with, if \c O_CREAT is included in \p flags.
 *
 * \return the new FileDescriptor.
 */
FileDescriptor FileDescriptor::create_open(const char *file, int flags, mode_t mode) {
	return FileDescriptor(file, flags, mode);
}

/**
 * \brief Constructs a new FileDescriptor by calling \c socket(2).
 *
 * \param[in] pf the protocol family to create a socket in.
 *
 * \param[in] type the type of socket to create.
 *
 * \param[in] proto the specific protocol to create a socket for, or 0 to use the default protocol for a given \c pf and \c type.
 *
 * \return the new FileDescriptor.
 */
FileDescriptor FileDescriptor::create_socket(int pf, int type, int proto) {
	return FileDescriptor(pf, type, proto);
}

/**
 * \brief Constructs a FileDescriptor that refers to a unique file that has not been opened by any other process, and that does not have any name on disk.
 *
 * \param[in] pattern the pattern for the filename, which must be suitable for \c mkstemp().
 *
 * \return the new descriptor.
 */
FileDescriptor FileDescriptor::create_temp(const char *pattern) {
	return FileDescriptor(pattern);
}

/**
 * \brief Constructs a FileDescriptor with no associated descriptor.
 */
FileDescriptor::FileDescriptor() : fd_(-1) {
}

/**
 * \brief Move-constructs a FileDescriptor.
 *
 * \param[in] moveref the descriptor to move from.
 */
FileDescriptor::FileDescriptor(FileDescriptor &&moveref) : fd_(moveref.fd_) {
	moveref.fd_ = -1;
}

/**
 * \brief Destroys a FileDescriptor.
 */
FileDescriptor::~FileDescriptor() {
	try {
		close();
	} catch (...) {
		// Swallow.
	}
}

/**
 * \brief Move-assigns one FileDescriptor to another.
 *
 * \param[in] moveref the descriptor to assign to this descriptor.
 *
 * \return this descriptor.
 */
FileDescriptor &FileDescriptor::operator=(FileDescriptor &&moveref) {
	close();
	fd_ = moveref.fd_;
	moveref.fd_ = -1;
	return *this;
}

/**
 * \brief Exchanges the contents of two FileDescriptor objects.
 *
 * \param[in,out] other the other descriptor to swap with.
 */
void FileDescriptor::swap(FileDescriptor &other) {
	std::swap(fd_, other.fd_);
}

/**
 * \brief Closes the descriptor.
 */
void FileDescriptor::close() {
	if (fd_ >= 0) {
		if (::close(fd_) < 0) {
			throw SystemError("close", errno);
		}
		fd_ = -1;
	}
}

/**
 * \brief Gets the actual file descriptor.
 *
 * \return the descriptor.
 */
int FileDescriptor::fd() const {
	return fd_;
}

/**
 * \brief Checks whether the file descriptor is valid.
 *
 * \return \c true if the descriptor is valid, or \c false if it has been closed or has not been initialized.
 */
bool FileDescriptor::is() const {
	return fd_ >= 0;
}

/**
 * \brief Sets whether the descriptor is blocking.
 *
 * \param[in] block \c true to set the descriptor to blocking mode, or \c false to set the descriptor to non-blocking mode.
 */
void FileDescriptor::set_blocking(bool block) const {
	long flags = fcntl(fd_, F_GETFL);
	if (flags < 0) {
		throw SystemError("fcntl", errno);
	}
	if (block) {
		flags &= ~O_NONBLOCK;
	} else {
		flags |= O_NONBLOCK;
	}
	if (fcntl(fd_, F_SETFL, flags) < 0) {
		throw SystemError("fcntl", errno);
	}
}

FileDescriptor::FileDescriptor(int fd) : fd_(fd) {
	if (fd_ < 0) {
		throw std::invalid_argument("Invalid file descriptor");
	}
}

FileDescriptor::FileDescriptor(const char *file, int flags, mode_t mode) : fd_(open(file, flags, mode)) {
	if (fd_ < 0) {
		if (errno == ENOENT) {
			throw FileNotFoundError();
		} else {
			throw SystemError("open", errno);
		}
	}
}

FileDescriptor::FileDescriptor(int af, int type, int proto) : fd_(socket(af, type, proto)) {
	if (fd_ < 0) {
		throw SystemError("socket", errno);
	}
}

FileDescriptor::FileDescriptor(const char *pattern) {
	const std::string &tmpdir = Glib::get_tmp_dir();
	std::vector<char> filename(tmpdir.begin(), tmpdir.end());
	filename.push_back('/');
	for (const char *ptr = pattern; *ptr; ++ptr) {
		filename.push_back(*ptr);
	}
	filename.push_back('\0');
	fd_ = mkstemp(&filename[0]);
	if (fd_ < 0) {
		throw SystemError("mkstemp", errno);
	}
	if (unlink(&filename[0]) < 0) {
		int saved_errno = errno;
		::close(fd_);
		throw SystemError("unlink", saved_errno);
	}
}

void std::swap(FileDescriptor &x, FileDescriptor &y) {
	x.swap(y);
}

