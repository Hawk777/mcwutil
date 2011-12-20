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

FileDescriptor FileDescriptor::create_from_fd(int fd) {
	return FileDescriptor(fd);
}

FileDescriptor FileDescriptor::create_open(const char *file, int flags, mode_t mode) {
	return FileDescriptor(file, flags, mode);
}

FileDescriptor FileDescriptor::create_socket(int pf, int type, int proto) {
	return FileDescriptor(pf, type, proto);
}

FileDescriptor FileDescriptor::create_temp(const char *pattern) {
	return FileDescriptor(pattern);
}

FileDescriptor::FileDescriptor() : fd_(-1) {
}

FileDescriptor::FileDescriptor(FileDescriptor &&moveref) : fd_(moveref.fd_) {
	moveref.fd_ = -1;
}

FileDescriptor::~FileDescriptor() {
	try {
		close();
	} catch (...) {
		// Swallow.
	}
}

FileDescriptor &FileDescriptor::operator=(FileDescriptor &&moveref) {
	close();
	fd_ = moveref.fd_;
	moveref.fd_ = -1;
	return *this;
}

void FileDescriptor::swap(FileDescriptor &other) {
	std::swap(fd_, other.fd_);
}

void FileDescriptor::close() {
	if (fd_ >= 0) {
		if (::close(fd_) < 0) {
			throw SystemError("close", errno);
		}
		fd_ = -1;
	}
}

int FileDescriptor::fd() const {
	return fd_;
}

bool FileDescriptor::is() const {
	return fd_ >= 0;
}

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

