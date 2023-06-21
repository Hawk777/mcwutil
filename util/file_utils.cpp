#include "util/file_utils.h"
#include "util/exception.h"
#include "util/fd.h"
#include <cerrno>
#include <cstddef>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void FileUtils::read(const FileDescriptor &fd, void *buf, std::size_t count) {
	uint8_t *pc = static_cast<uint8_t *>(buf);
	while(count) {
		ssize_t rc = ::read(fd.fd(), pc, count);
		if(rc < 0) {
			throw SystemError("read", errno);
		} else if(!rc) {
			throw std::runtime_error("read: unexpected EOF");
		} else {
			pc += rc;
			count -= rc;
		}
	}
}

void FileUtils::write(const FileDescriptor &fd, const void *buf, std::size_t count) {
	const uint8_t *pc = static_cast<const uint8_t *>(buf);
	while(count) {
		ssize_t rc = ::write(fd.fd(), pc, count);
		if(rc < 0) {
			throw SystemError("write", errno);
		} else {
			pc += rc;
			count -= rc;
		}
	}
}

void FileUtils::pread(const FileDescriptor &fd, void *buf, std::size_t count, off_t offset) {
	uint8_t *pc = static_cast<uint8_t *>(buf);
	while(count) {
		ssize_t rc = ::pread(fd.fd(), pc, count, offset);
		if(rc < 0) {
			throw SystemError("pread", errno);
		} else if(!rc) {
			throw std::runtime_error("pread: unexpected EOF");
		} else {
			pc += rc;
			count -= rc;
			offset += rc;
		}
	}
}

void FileUtils::pwrite(const FileDescriptor &fd, const void *buf, std::size_t count, off_t offset) {
	const uint8_t *pc = static_cast<const uint8_t *>(buf);
	while(count) {
		ssize_t rc = ::pwrite(fd.fd(), pc, count, offset);
		if(rc < 0) {
			throw SystemError("pwrite", errno);
		} else {
			pc += rc;
			count -= rc;
			offset += rc;
		}
	}
}

void FileUtils::fstat(const FileDescriptor &fd, struct stat &stbuf) {
	if(::fstat(fd.fd(), &stbuf) < 0) {
		throw SystemError("fstat", errno);
	}
}

void FileUtils::ftruncate(const FileDescriptor &fd, off_t length) {
	if(::ftruncate(fd.fd(), length) < 0) {
		throw SystemError("ftruncate", errno);
	}
}
