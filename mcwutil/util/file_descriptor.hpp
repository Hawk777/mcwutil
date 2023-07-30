#ifndef UTIL_FILE_DESCRIPTOR_H
#define UTIL_FILE_DESCRIPTOR_H

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace mcwutil {
/**
 * \brief A file descriptor that is safely closed on destruction.
 */
class file_descriptor {
	public:
	static file_descriptor create_open(const char *file, int flags, mode_t mode);
	static file_descriptor create_open(const std::filesystem::path &file, int flags, mode_t mode);

	explicit file_descriptor();
	explicit file_descriptor(file_descriptor &&moveref);
	~file_descriptor();
	file_descriptor &operator=(file_descriptor &&moveref);

	int fd() const;
	void close();
	void read(void *buf, std::size_t count) const;
	void write(const void *buf, std::size_t count) const;
	void pread(void *buf, std::size_t count, off_t offset) const;
	void pwrite(const void *buf, std::size_t count, off_t offset) const;
	void fstat(struct stat &stbuf) const;
	void ftruncate(off_t length) const;

	private:
	/**
	 * \brief The raw file descriptor value, or −1 if this object has been
	 * closed or moved from.
	 */
	int fd_;

	explicit file_descriptor(const char *file, int flags, mode_t mode);
};
}

#endif
