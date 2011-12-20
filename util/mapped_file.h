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
		/**
		 * \brief Maps in a file.
		 *
		 * \param[in] fd the file to map.
		 *
		 * \param[in] prot the protection mode to use.
		 *
		 * \param[in] flags the mapping flags to use.
		 */
		MappedFile(const FileDescriptor &fd, int prot = PROT_READ, int flags = MAP_SHARED | MAP_FILE);

		/**
		 * \brief Maps in a file.
		 *
		 * \param[in] filename the file to map.
		 *
		 * \param[in] prot the protection mode to use.
		 *
		 * \param[in] flags the mapping flags to use.
		 */
		MappedFile(const std::string &filename, int prot = PROT_READ, int flags = MAP_SHARED | MAP_FILE);

		/**
		 * \brief Unmaps the file.
		 */
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

		/**
		 * \brief Forces changes made to a writable mapping back to the disk.
		 */
		void sync();

	private:
		void *data_;
		std::size_t size_;
};

#endif

