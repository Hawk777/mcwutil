#include <mcwutil/util/file_descriptor.hpp>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

using mcwutil::file_descriptor;
using namespace std::literals::string_literals;

/**
 * \brief Constructs a new file_descriptor by calling \c open(2).
 *
 * \param[in] file the name of the file to open or create.
 *
 * \param[in] flags the file flags to use as per \c open(2).
 *
 * \param[in] mode the permissions to create a new file with, if \c O_CREAT is included in \p flags.
 *
 * \return the new file_descriptor.
 */
file_descriptor file_descriptor::create_open(const char *file, int flags, mode_t mode) {
	return file_descriptor(file, flags, mode);
}

/**
 * \brief Constructs a new file_descriptor by calling \c open(2).
 *
 * \param[in] file the name of the file to open or create.
 *
 * \param[in] flags the file flags to use as per \c open(2).
 *
 * \param[in] mode the permissions to create a new file with, if \c O_CREAT is included in \p flags.
 *
 * \return the new file_descriptor.
 */
file_descriptor file_descriptor::create_open(const std::filesystem::path &file, int flags, mode_t mode) {
	return create_open(file.c_str(), flags, mode);
}

/**
 * \brief Constructs a file_descriptor with no associated descriptor.
 */
file_descriptor::file_descriptor() :
		fd_(-1) {
}

/**
 * \brief Move-constructs a file_descriptor.
 *
 * \param[in] moveref the descriptor to move from.
 */
file_descriptor::file_descriptor(file_descriptor &&moveref) :
		fd_(moveref.fd_) {
	moveref.fd_ = -1;
}

/**
 * \brief Destroys a file_descriptor.
 */
file_descriptor::~file_descriptor() {
	try {
		close();
	} catch(...) {
		// Swallow.
	}
}

/**
 * \brief Move-assigns one file_descriptor to another.
 *
 * \param[in] moveref the descriptor to assign to this descriptor.
 *
 * \return this descriptor.
 */
file_descriptor &file_descriptor::operator=(file_descriptor &&moveref) {
	close();
	fd_ = moveref.fd_;
	moveref.fd_ = -1;
	return *this;
}

/**
 * \brief Gets the actual file descriptor.
 *
 * \return the descriptor.
 */
int file_descriptor::fd() const {
	return fd_;
}

/**
 * \brief Closes the descriptor.
 */
void file_descriptor::close() {
	if(fd_ >= 0) {
		if(::close(fd_) < 0) {
			throw std::system_error(errno, std::system_category(), "close"s);
		}
		fd_ = -1;
	}
}

/**
 * \brief Reads data from the current file position.
 *
 * \pre this descriptor is open.
 *
 * \param[out] buf where to store the read bytes.
 *
 * \param[in] count the number of bytes to read.
 */
void file_descriptor::read(void *buf, std::size_t count) const {
	uint8_t *pc = static_cast<uint8_t *>(buf);
	while(count) {
		ssize_t rc = ::read(fd_, pc, count);
		if(rc < 0) {
			throw std::system_error(errno, std::system_category(), "read");
		} else if(!rc) {
			throw std::runtime_error("read: unexpected EOF");
		} else {
			pc += rc;
			count -= rc;
		}
	}
}

/**
 * \brief Writes data to the current file position.
 *
 * \pre this descriptor is open.
 *
 * \param[out] buf the data to write.
 *
 * \param[in] count the number of bytes to write.
 */
void file_descriptor::write(const void *buf, std::size_t count) const {
	const uint8_t *pc = static_cast<const uint8_t *>(buf);
	while(count) {
		ssize_t rc = ::write(fd_, pc, count);
		if(rc < 0) {
			throw std::system_error(errno, std::system_category(), "write");
		} else {
			pc += rc;
			count -= rc;
		}
	}
}

/**
 * \brief Reads data from an arbitrary position.
 *
 * \pre this descriptor is open.
 *
 * \param[out] buf where to store the read bytes.
 *
 * \param[in] count the number of bytes to read.
 *
 * \param[in] offset the position in the file at which to begin reading.
 */
void file_descriptor::pread(void *buf, std::size_t count, off_t offset) const {
	uint8_t *pc = static_cast<uint8_t *>(buf);
	while(count) {
		ssize_t rc = ::pread(fd_, pc, count, offset);
		if(rc < 0) {
			throw std::system_error(errno, std::system_category(), "pread");
		} else if(!rc) {
			throw std::runtime_error("pread: unexpected EOF");
		} else {
			pc += rc;
			count -= rc;
			offset += rc;
		}
	}
}

/**
 * \brief Writes data to an arbitrary position.
 *
 * \pre this descriptor is open.
 *
 * \param[out] buf the data to write.
 *
 * \param[in] count the number of bytes to write.
 *
 * \param[in] offset the position in the file at which to begin writing.
 */
void file_descriptor::pwrite(const void *buf, std::size_t count, off_t offset) const {
	const uint8_t *pc = static_cast<const uint8_t *>(buf);
	while(count) {
		ssize_t rc = ::pwrite(fd_, pc, count, offset);
		if(rc < 0) {
			throw std::system_error(errno, std::system_category(), "pwrite");
		} else {
			pc += rc;
			count -= rc;
			offset += rc;
		}
	}
}

/**
 * \brief Obtains file metadata.
 *
 * \pre this descriptor is open.
 *
 * \param[out] stbuf where to store the metadata.
 */
void file_descriptor::fstat(struct stat &stbuf) const {
	if(::fstat(fd_, &stbuf) < 0) {
		throw std::system_error(errno, std::system_category(), "fstat");
	}
}

/**
 * \brief Changes the length of the file.
 *
 * \pre this descriptor is open.
 *
 * \param[in] length the new length of the file, in bytes.
 */
void file_descriptor::ftruncate(off_t length) const {
	if(::ftruncate(fd_, length) < 0) {
		throw std::system_error(errno, std::system_category(), "ftruncate");
	}
}

/**
 * \brief Constructs a new file_descriptor by calling \c open(2).
 *
 * \param[in] file the name of the file to open or create.
 *
 * \param[in] flags the file flags to use as per \c open(2).
 *
 * \param[in] mode the permissions to create a new file with, if \c O_CREAT is included in \p flags.
 */
file_descriptor::file_descriptor(const char *file, int flags, mode_t mode) :
		fd_(open(file, flags, mode)) {
	if(fd_ < 0) {
		std::ostringstream msg;
		msg << "open(" << file << ")";
		throw std::system_error(errno, std::system_category(), msg.str());
	}
}
