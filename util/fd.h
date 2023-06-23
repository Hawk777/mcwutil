#ifndef UTIL_FD_H
#define UTIL_FD_H

#include <algorithm>
#include <filesystem>
#include <sys/types.h>

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

	void swap(FileDescriptor &other);
	void close();
	int fd() const;
	bool is() const;

	private:
	int fd_;

	explicit FileDescriptor(const char *file, int flags, mode_t mode);
};

namespace std {
void swap(FileDescriptor &x, FileDescriptor &y);
}

#endif
