#include "util/mapped_file.h"
#include "util/misc.h"
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

/**
 * \brief Maps in a file.
 *
 * \param[in] fd the file to map.
 *
 * \param[in] prot the protection mode to use.
 *
 * \param[in] flags the mapping flags to use.
 */
MappedFile::MappedFile(const FileDescriptor &fd, int prot, int flags) {
	struct stat st;
	if(fstat(fd.fd(), &st) < 0) {
		throw std::system_error(errno, std::system_category(), "fstat");
	}
	if(static_cast<uintmax_t>(st.st_size) > std::numeric_limits<std::size_t>::max()) {
		throw std::runtime_error("File too large to map into virtual address space");
	}
	size_ = static_cast<std::size_t>(st.st_size);
	if(size_) {
		data_ = mmap(0, size_, prot, flags, fd.fd(), 0);
		if(data_ == get_map_failed()) {
			throw std::system_error(errno, std::system_category(), "mmap");
		}
	} else {
		data_ = 0;
	}
}

/**
 * \brief Unmaps the file.
 */
MappedFile::~MappedFile() {
	if(data_) {
		munmap(data_, size_);
	}
}

/**
 * \brief Forces changes made to a writable mapping back to the disk.
 */
void MappedFile::sync() {
	msync(data_, size_, MS_SYNC | MS_INVALIDATE);
}
