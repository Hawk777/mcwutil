#ifndef UTIL_MAPPED_FILE_H
#define UTIL_MAPPED_FILE_H

#include "util/fd.h"
#include "util/non_copyable.h"
#include <string>
#include <sys/mman.h>

/**
 * \brief A memory-mapped view of a file.
 */
class MappedFile : public NonCopyable {
	public:
	explicit MappedFile(const FileDescriptor &fd, int prot = PROT_READ, int flags = MAP_SHARED | MAP_FILE);
	explicit MappedFile(const std::string &filename, int prot = PROT_READ, int flags = MAP_SHARED | MAP_FILE);
	~MappedFile();

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

#endif
