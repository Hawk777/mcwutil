#include <mcwutil/util/file_descriptor.hpp>
#include <mcwutil/zlib_utils.hpp>
#include <fcntl.h>
#include <iostream>
#include <new>
#include <stdexcept>
#include <vector>
#include <zlib.h>

/**
 * \brief Entry point for the \c zlib-compress utility.
 *
 * \param[in] appname The name of the application.
 *
 * \param[in] args the command-line arguments.
 *
 * \return the application exit code.
 */
int mcwutil::zlib::compress(std::string_view appname, std::span<char *> args) {
	// Check parameters.
	if(args.size() != 2) {
		std::cerr << "Usage:\n";
		std::cerr << appname << " zlib-compress inputfile outputfile\n";
		std::cerr << '\n';
		std::cerr << "Compresses a file with zlib.\n";
		std::cerr << '\n';
		std::cerr << "Arguments:\n";
		std::cerr << "  inputfile - the file to compress\n";
		std::cerr << "  outputfile - the file to compress into\n";
		return 1;
	}

	// Read input file.
	file_descriptor input_fd = file_descriptor::create_open(args[0], O_RDONLY, 0);
	struct stat stbuf;
	input_fd.fstat(stbuf);
	unsigned char input_buffer[stbuf.st_size];
	input_fd.read(input_buffer, sizeof(input_buffer));

	// Compress data.
	unsigned char output_buffer[compressBound(sizeof(input_buffer))];
	unsigned long output_length = sizeof(output_buffer);
	int zlib_rc = compress2(output_buffer, &output_length, input_buffer, sizeof(input_buffer), 9);
	switch(zlib_rc) {
		case Z_OK:
			break;
		case Z_MEM_ERROR:
			throw std::bad_alloc();
		case Z_BUF_ERROR:
			throw std::logic_error("Internal error: supposedly-sufficient compression buffer was insufficient.");
		case Z_STREAM_ERROR:
			throw std::logic_error("Internal error: known-valid compression level was invalid.");
		default:
			throw std::logic_error("Internal error: compress2 returned unknown error code.");
	}

	// Write output file.
	file_descriptor output_fd = file_descriptor::create_open(args[1], O_WRONLY | O_TRUNC | O_CREAT, 0666);
	output_fd.write(output_buffer, output_length);
	output_fd.close();

	return 0;
}

/**
 * \brief Entry point for the \c zlib-decompress utility.
 *
 * \param[in] appname The name of the application.
 *
 * \param[in] args the command-line arguments.
 *
 * \return the application exit code.
 */
int mcwutil::zlib::decompress(std::string_view appname, std::span<char *> args) {
	// Check parameters.
	if(args.size() != 2) {
		std::cerr << "Usage:\n";
		std::cerr << appname << " zlib-decompress inputfile outputfile\n";
		std::cerr << '\n';
		std::cerr << "Decompresses a zlib-compressed file.\n";
		std::cerr << '\n';
		std::cerr << "Arguments:\n";
		std::cerr << "  inputfile - the file to decompress\n";
		std::cerr << "  outputfile - the file to decompress into\n";
		return 1;
	}

	// Read input file.
	file_descriptor input_fd = file_descriptor::create_open(args[0], O_RDONLY, 0);
	struct stat stbuf;
	input_fd.fstat(stbuf);
	unsigned char input_buffer[stbuf.st_size];
	input_fd.read(input_buffer, sizeof(input_buffer));

	// Decompress data.
	std::vector<unsigned char> output_buffer(sizeof(input_buffer) * 4);
	bool done = false;
	while(!done) {
		unsigned long output_length = output_buffer.size();
		int zlib_rc = uncompress(&output_buffer[0], &output_length, input_buffer, sizeof(input_buffer));
		switch(zlib_rc) {
			case Z_OK:
				output_buffer.resize(output_length);
				done = true;
				break;

			case Z_MEM_ERROR:
				throw std::bad_alloc();

			case Z_BUF_ERROR:
				output_buffer.resize(output_buffer.size() * 2);
				break;

			case Z_DATA_ERROR:
				throw std::runtime_error("uncompress: malformed zlib stream.");

			default:
				throw std::logic_error("Internal error: uncompress returned unknown error code.");
		}
	}

	// Write output file.
	file_descriptor output_fd = file_descriptor::create_open(args[1], O_WRONLY | O_TRUNC | O_CREAT, 0666);
	output_fd.write(&output_buffer[0], output_buffer.size());
	output_fd.close();

	return 0;
}

/**
 * \brief Entry point for the \c zlib-check utility.
 *
 * \param[in] appname The name of the application.
 *
 * \param[in] args the command-line arguments.
 *
 * \return the application exit code.
 */
int mcwutil::zlib::check(std::string_view appname, std::span<char *> args) {
	// Check parameters.
	if(args.size() != 1) {
		std::cerr << "Usage:\n";
		std::cerr << appname << " zlib-check inputfile\n";
		std::cerr << '\n';
		std::cerr << "Decompresses a zlib-compressed file, discarding the contents.\n";
		std::cerr << '\n';
		std::cerr << "Arguments:\n";
		std::cerr << "  inputfile - the file to decompress\n";
		return 1;
	}

	// Read input file.
	file_descriptor input_fd = file_descriptor::create_open(args[0], O_RDONLY, 0);
	struct stat stbuf;
	input_fd.fstat(stbuf);
	unsigned char input_buffer[stbuf.st_size];
	input_fd.read(input_buffer, sizeof(input_buffer));

	// Decompress data.
	std::vector<unsigned char> output_buffer(sizeof(input_buffer) * 4);
	bool done = false;
	while(!done) {
		unsigned long output_length = output_buffer.size();
		int zlib_rc = uncompress(&output_buffer[0], &output_length, input_buffer, sizeof(input_buffer));
		switch(zlib_rc) {
			case Z_OK:
				output_buffer.resize(output_length);
				done = true;
				break;

			case Z_MEM_ERROR:
				throw std::bad_alloc();

			case Z_BUF_ERROR:
				output_buffer.resize(output_buffer.size() * 2);
				break;

			case Z_DATA_ERROR:
				throw std::runtime_error("uncompress: malformed zlib stream.");

			default:
				throw std::logic_error("Internal error: uncompress returned unknown error code.");
		}
	}

	return 0;
}
