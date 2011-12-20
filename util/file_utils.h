#ifndef UTIL_FILE_UTILS_H
#define UTIL_FILE_UTILS_H

#include "util/fd.h"
#include <cstddef>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace FileUtils {
	void read(const FileDescriptor &fd, void *buf, std::size_t count);
	void write(const FileDescriptor &fd, const void *buf, std::size_t count);
	void pread(const FileDescriptor &fd, void *buf, std::size_t count, off_t offset);
	void pwrite(const FileDescriptor &fd, const void *buf, std::size_t count, off_t offset);
	void fstat(const FileDescriptor &fd, struct stat &stbuf);
	void ftruncate(const FileDescriptor &fd, off_t length);
}

#endif

