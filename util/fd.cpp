#include "util/fd.h"
#include <cstdlib>
#include <fcntl.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

using namespace std::literals::string_literals;

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
FileDescriptor FileDescriptor::create_open(const char *file, int flags, mode_t mode) {
	return FileDescriptor(file, flags, mode);
}

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
FileDescriptor FileDescriptor::create_open(const std::filesystem::path &file, int flags, mode_t mode) {
	return create_open(file.c_str(), flags, mode);
}

/**
 * \brief Constructs a FileDescriptor with no associated descriptor.
 */
FileDescriptor::FileDescriptor() :
		fd_(-1) {
}

/**
 * \brief Move-constructs a FileDescriptor.
 *
 * \param[in] moveref the descriptor to move from.
 */
FileDescriptor::FileDescriptor(FileDescriptor &&moveref) :
		fd_(moveref.fd_) {
	moveref.fd_ = -1;
}

/**
 * \brief Destroys a FileDescriptor.
 */
FileDescriptor::~FileDescriptor() {
	try {
		close();
	} catch(...) {
		// Swallow.
	}
}

/**
 * \brief Move-assigns one FileDescriptor to another.
 *
 * \param[in] moveref the descriptor to assign to this descriptor.
 *
 * \return this descriptor.
 */
FileDescriptor &FileDescriptor::operator=(FileDescriptor &&moveref) {
	close();
	fd_ = moveref.fd_;
	moveref.fd_ = -1;
	return *this;
}

/**
 * \brief Closes the descriptor.
 */
void FileDescriptor::close() {
	if(fd_ >= 0) {
		if(::close(fd_) < 0) {
			throw std::system_error(errno, std::system_category(), "close"s);
		}
		fd_ = -1;
	}
}

/**
 * \brief Gets the actual file descriptor.
 *
 * \return the descriptor.
 */
int FileDescriptor::fd() const {
	return fd_;
}

FileDescriptor::FileDescriptor(const char *file, int flags, mode_t mode) :
		fd_(open(file, flags, mode)) {
	if(fd_ < 0) {
		std::ostringstream msg;
		msg << "open(" << file << ")";
		throw std::system_error(errno, std::system_category(), msg.str());
	}
}
