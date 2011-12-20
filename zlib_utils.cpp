#include "zlib_utils.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/file_utils.h"
#include "util/globals.h"
#include <fcntl.h>
#include <iostream>
#include <string>
#include <vector>
#include <zlib.h>

int ZLib::compress(const std::vector<std::string> &args) {
	// Check parameters.
	if (args.size() != 2) {
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
	FileDescriptor input_fd = FileDescriptor::create_open(args[0].c_str(), O_RDONLY, 0);
	struct stat stbuf;
	FileUtils::fstat(input_fd, stbuf);
	unsigned char input_buffer[stbuf.st_size];
	FileUtils::read(input_fd, input_buffer, sizeof(input_buffer));

	// Compress data.
	unsigned char output_buffer[compressBound(sizeof(input_buffer))];
	unsigned long output_length = sizeof(output_buffer);
	int zlib_rc = compress2(output_buffer, &output_length, input_buffer, sizeof(input_buffer), 9);
	switch (zlib_rc) {
		case Z_OK: break;
		case Z_MEM_ERROR: throw SystemError("compress2", ENOMEM);
		case Z_BUF_ERROR: throw std::logic_error("Internal error: supposedly-sufficient compression buffer was insufficient.");
		case Z_STREAM_ERROR: throw std::logic_error("Internal error: known-valid compression level was invalid.");
		default: throw std::logic_error("Internal error: compress2 returned unknown error code.");
	}

	// Write output file.
	FileDescriptor output_fd = FileDescriptor::create_open(args[1].c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
	FileUtils::write(output_fd, output_buffer, output_length);

	return 0;
}

int ZLib::decompress(const std::vector<std::string> &args) {
	// Check parameters.
	if (args.size() != 2) {
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
	FileDescriptor input_fd = FileDescriptor::create_open(args[0].c_str(), O_RDONLY, 0);
	struct stat stbuf;
	FileUtils::fstat(input_fd, stbuf);
	unsigned char input_buffer[stbuf.st_size];
	FileUtils::read(input_fd, input_buffer, sizeof(input_buffer));

	// Decompress data.
	std::vector<unsigned char> output_buffer(sizeof(input_buffer) * 4);
	bool done = false;
	while (!done) {
		unsigned long output_length = output_buffer.size();
		int zlib_rc = uncompress(&output_buffer[0], &output_length, input_buffer, sizeof(input_buffer));
		switch (zlib_rc) {
			case Z_OK:
				output_buffer.resize(output_length);
				done = true;
				break;

			case Z_MEM_ERROR:
				throw SystemError("compress2", ENOMEM);

			case Z_BUF_ERROR:
				output_buffer.resize(output_buffer.size() * 2);
				break;

			case Z_DATA_ERROR:
				throw std::runtime_error("uncompress: malformed zlib stream.");

			default:
				throw std::logic_error("Internal error: compress2 returned unknown error code.");
		}
	}

	// Write output file.
	FileDescriptor output_fd = FileDescriptor::create_open(args[1].c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
	FileUtils::write(output_fd, &output_buffer[0], output_buffer.size());

	return 0;
}

