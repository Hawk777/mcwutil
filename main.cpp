#include "zlib_utils.h"
#include "nbt/nbt.h"
#include "region/pack.h"
#include "region/unpack.h"
#include "util/globals.h"
#include <iostream>
#include <locale>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>
#include <glibmm/convert.h>
#include <glibmm/exception.h>
#include <glibmm/ustring.h>

namespace {
	void usage() {
		std::cerr << "Usage:\n";
		std::cerr << appname << " command [arguments...]\n";
		std::cerr << '\n';
		std::cerr << "Possible commands are:\n";
		std::cerr << "  region-unpack - unpacks the chunks from a region file (.mca or .mcr)\n";
		std::cerr << "  region-pack - packs chunks into a region file (.mca or .mcr)\n";
		std::cerr << "  zlib-decompress - decompresses a ZLIB-format file\n";
		std::cerr << "  zlib-compress - compresses a ZLIB-format file\n";
		std::cerr << "  zlib-check - decompresses a ZLIB-format file, discarding the contents\n";
		std::cerr << "  nbt-to-xml - converts an NBT file to an equivalent XML file\n";
		std::cerr << "  nbt-from-xml - converts an NBT-equivalent XML file to an NBT file\n";
		std::cerr << "  nbt-patch-barray - replaces specific byte values in NBT byte arrays with other values\n";
	}

	int main_impl(int argc, char **argv) {
		// Set the current locale from environment variables.
		std::locale::global(std::locale(""));

		// Extract the application name.
		appname = Glib::locale_to_utf8(argv[0]);

		// Extract the command name.
		if (argc < 2) {
			usage();
			return 1;
		}
		const Glib::ustring &command = Glib::locale_to_utf8(argv[1]);

		// Extract the remaining command-line arguments.
		std::vector<std::string> args;
		args.reserve(argc - 2);
		for (int i = 2; i < argc; ++i) {
			args.push_back(argv[i]);
		}

		// Dispatch.
		if (command == u8"region-unpack") {
			return Region::unpack(args);
		} else if (command == u8"region-pack") {
			return Region::pack(args);
		} else if (command == u8"zlib-decompress") {
			return ZLib::decompress(args);
		} else if (command == u8"zlib-compress") {
			return ZLib::compress(args);
		} else if (command == u8"zlib-check") {
			return ZLib::check(args);
		} else if (command == u8"nbt-to-xml") {
			return NBT::to_xml(args);
		} else if (command == u8"nbt-from-xml") {
			return NBT::from_xml(args);
		} else if (command == u8"nbt-patch-barray") {
			return NBT::patch_barray(args);
		} else {
			usage();
		}

		return 0;
	}
}

int main(int argc, char **argv) {
	try {
		return main_impl(argc, argv);
	} catch (const Glib::Exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch (const std::exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch (...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}

