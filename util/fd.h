#ifndef UTIL_FD_H
#define UTIL_FD_H

#include <algorithm>
#include <sys/types.h>

/**
 * \brief A file descriptor that is safely closed on destruction.
 */
class FileDescriptor {
	public:
	static FileDescriptor create_from_fd(int fd);
	static FileDescriptor create_open(const char *file, int flags, mode_t mode);
	static FileDescriptor create_socket(int pf, int type, int proto);
	static FileDescriptor create_temp(const char *pattern);

	explicit FileDescriptor();
	explicit FileDescriptor(FileDescriptor &&moveref);
	~FileDescriptor();
	FileDescriptor &operator=(FileDescriptor &&moveref);

	void swap(FileDescriptor &other);
	void close();
	int fd() const;
	bool is() const;
	void set_blocking(bool block) const;

	private:
	int fd_;

	explicit FileDescriptor(int fd);
	explicit FileDescriptor(const char *file, int flags, mode_t mode);
	explicit FileDescriptor(int pf, int type, int proto);
	explicit FileDescriptor(const char *pattern);
};

namespace std {
void swap(FileDescriptor &x, FileDescriptor &y);
}

#endif
