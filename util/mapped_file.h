#ifndef UTIL_MAPPED_FILE_H
#define UTIL_MAPPED_FILE_H

#include <cstddef>
#include <sys/mman.h>

namespace mcwutil {
class FileDescriptor;

/**
 * \brief A memory-mapped view of a file.
 */
class MappedFile {
	public:
	explicit MappedFile(const FileDescriptor &fd, int prot = PROT_READ, int flags = MAP_SHARED | MAP_FILE);
	~MappedFile();

	// This class is not copyable.
	explicit MappedFile(const MappedFile &) = delete;
	void operator=(const MappedFile &) = delete;

	/**
	 * \brief Returns the mapped data.
	 *
	 * \return the mapped data.
	 */
	void *data() {
		return data_;
	}

	/**
	 * \brief Returns the mapped data.
	 *
	 * \return the mapped data.
	 */
	const void *data() const {
		return data_;
	}

	/**
	 * \brief Returns the size of the file.
	 *
	 * \return the size of the file.
	 */
	std::size_t size() const {
		return size_;
	}

	void sync();

	private:
	void *data_;
	std::size_t size_;
};
}

#endif
