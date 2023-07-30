#ifndef UTIL_MAPPED_FILE_H
#define UTIL_MAPPED_FILE_H

#include <cstddef>
#include <sys/mman.h>

namespace mcwutil {
class file_descriptor;

/**
 * \brief A memory-mapped view of a file.
 */
class mapped_file {
	public:
	explicit mapped_file(const file_descriptor &fd, int prot = PROT_READ, int flags = MAP_SHARED | MAP_FILE);
	~mapped_file();

	// This class is not copyable.
	explicit mapped_file(const mapped_file &) = delete;
	void operator=(const mapped_file &) = delete;

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
	/**
	 * \brief The base address of the memory-mapped region.
	 */
	void *data_;

	/**
	 * \brief The size of the file.
	 */
	std::size_t size_;
};
}

#endif
