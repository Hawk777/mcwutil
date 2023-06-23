#ifndef UTIL_FD_H
#define UTIL_FD_H

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
class FileDescriptor {
	public:
	static FileDescriptor create_open(const char *file, int flags, mode_t mode);
	static FileDescriptor create_open(const std::filesystem::path &file, int flags, mode_t mode);

	explicit FileDescriptor();
	explicit FileDescriptor(FileDescriptor &&moveref);
	~FileDescriptor();
	FileDescriptor &operator=(FileDescriptor &&moveref);

	int fd() const;
	void close();
	void read(void *buf, std::size_t count) const;
	void write(const void *buf, std::size_t count) const;
	void pread(void *buf, std::size_t count, off_t offset) const;
	void pwrite(const void *buf, std::size_t count, off_t offset) const;
	void fstat(struct stat &stbuf) const;
	void ftruncate(off_t length) const;

	private:
	int fd_;

	explicit FileDescriptor(const char *file, int flags, mode_t mode);
};
}

#endif
