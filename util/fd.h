#ifndef UTIL_FD_H
#define UTIL_FD_H

#include "util/non_copyable.h"
#include <algorithm>
#include <sys/types.h>

/**
 * \brief A file descriptor that is safely closed on destruction.
 */
class FileDescriptor : public NonCopyable {
	public:
		/**
		 * \brief Constructs a new FileDescriptor with a descriptor.
		 *
		 * \param[in] fd the existing file descriptor, of which ownership is taken.
		 *
		 * \return a new FileDescriptor owning \p fd.
		 */
		static FileDescriptor create_from_fd(int fd);

		/**
		 * \brief Constructs a new FileDescriptor by calling \c open(2).
		 *
		 * \param[in] file the name of the file to open or create.
		 *
		 * \param[in] flags the file flags to use as per \c open(2).
		 *
		 * \param[in] mode the permissions to create a new file with, if \c O_CREAT is included in \p flags.
		 *
		 * \return the new FileDescriptor.
		 */
		static FileDescriptor create_open(const char *file, int flags, mode_t mode);

		/**
		 * \brief Constructs a new FileDescriptor by calling \c socket(2).
		 *
		 * \param[in] pf the protocol family to create a socket in.
		 *
		 * \param[in] type the type of socket to create.
		 *
		 * \param[in] proto the specific protocol to create a socket for, or 0 to use the default protocol for a given \c pf and \c type.
		 *
		 * \return the new FileDescriptor.
		 */
		static FileDescriptor create_socket(int pf, int type, int proto);

		/**
		 * \brief Constructs a FileDescriptor that refers to a unique file that has not been opened by any other process, and that does not have any name on disk.
		 *
		 * \param[in] pattern the pattern for the filename, which must be suitable for \c mkstemp().
		 *
		 * \return the new descriptor.
		 */
		static FileDescriptor create_temp(const char *pattern);

		/**
		 * \brief Constructs a FileDescriptor with no associated descriptor.
		 */
		FileDescriptor();

		/**
		 * \brief Move-constructs a FileDescriptor.
		 *
		 * \param[in] moveref the descriptor to move from.
		 */
		FileDescriptor(FileDescriptor &&moveref);

		/**
		 * \brief Destroys a FileDescriptor.
		 */
		~FileDescriptor();

		/**
		 * \brief Move-assigns one FileDescriptor to another.
		 *
		 * \param[in] moveref the descriptor to assign to this descriptor.
		 *
		 * \return this descriptor.
		 */
		FileDescriptor &operator=(FileDescriptor &&moveref);

		/**
		 * \brief Exchanges the contents of two FileDescriptor objects.
		 *
		 * \param[in,out] other the other descriptor to swap with.
		 */
		void swap(FileDescriptor &other);

		/**
		 * \brief Closes the descriptor.
		 */
		void close();

		/**
		 * \brief Gets the actual file descriptor.
		 *
		 * \return the descriptor.
		 */
		int fd() const;

		/**
		 * \brief Checks whether the file descriptor is valid.
		 *
		 * \return \c true if the descriptor is valid, or \c false if it has been closed or has not been initialized.
		 */
		bool is() const;

		/**
		 * \brief Sets whether the descriptor is blocking.
		 *
		 * \param[in] block \c true to set the descriptor to blocking mode, or \c false to set the descriptor to non-blocking mode.
		 */
		void set_blocking(bool block) const;

	private:
		int fd_;

		FileDescriptor(int fd);
		FileDescriptor(const char *file, int flags, mode_t mode);
		FileDescriptor(int pf, int type, int proto);
		FileDescriptor(const char *pattern);
};

namespace std {
	void swap(FileDescriptor &x, FileDescriptor &y);
}

#endif

