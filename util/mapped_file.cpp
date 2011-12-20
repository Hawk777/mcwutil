#include "util/mapped_file.h"
#include "util/exception.h"
#include "util/misc.h"
#include <fcntl.h>
#include <limits>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

MappedFile::MappedFile(const FileDescriptor &fd, int prot, int flags) {
	struct stat st;
	if (fstat(fd.fd(), &st) < 0) {
		throw SystemError("fstat", errno);
	}
	if (static_cast<uintmax_t>(st.st_size) > std::numeric_limits<std::size_t>::max()) {
		throw std::runtime_error("File too large to map into virtual address space");
	}
	size_ = static_cast<std::size_t>(st.st_size);
	if (size_) {
		data_ = mmap(0, size_, prot, flags, fd.fd(), 0);
		if (data_ == get_map_failed()) {
			throw SystemError("mmap", errno);
		}
	} else {
		data_ = 0;
	}
}

MappedFile::MappedFile(const std::string &filename, int prot, int flags) {
	FileDescriptor fd = FileDescriptor::create_open(filename.c_str(), O_RDONLY, 0);
	struct stat st;
	if (fstat(fd.fd(), &st) < 0) {
		throw SystemError("fstat", errno);
	}
	if (static_cast<uintmax_t>(st.st_size) > std::numeric_limits<std::size_t>::max()) {
		throw std::runtime_error("File too large to map into virtual address space");
	}
	size_ = static_cast<std::size_t>(st.st_size);
	if (size_) {
		data_ = mmap(0, size_, prot, flags, fd.fd(), 0);
		if (data_ == get_map_failed()) {
			throw SystemError("mmap", errno);
		}
	} else {
		data_ = 0;
	}
}

MappedFile::~MappedFile() {
	if (data_) {
		munmap(data_, size_);
	}
}

void MappedFile::sync() {
	msync(data_, size_, MS_SYNC | MS_INVALIDATE);
}

