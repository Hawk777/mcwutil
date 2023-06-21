#include "coord_calc/coord_calc.h"
#include "nbt/nbt.h"
#include "region/pack.h"
#include "region/unpack.h"
#include "util/globals.h"
#include "zlib_utils.h"
#include <exception>
#include <glibmm/convert.h>
#include <glibmm/exception.h>
#include <iostream>
#include <locale>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <typeinfo>

namespace {
void usage() {
	std::cerr << "Usage:\n";
	std::cerr << appname << " command [arguments...]\n";
	std::cerr << '\n';
	std::cerr << "Possible commands are:\n";
	std::cerr << "  coord-calc - computes various useful numbers from a coordinate pair\n";
	std::cerr << "  region-unpack - unpacks the chunks from a region file (.mca or .mcr)\n";
	std::cerr << "  region-pack - packs chunks into a region file (.mca or .mcr)\n";
	std::cerr << "  zlib-decompress - decompresses a ZLIB-format file\n";
	std::cerr << "  zlib-compress - compresses a ZLIB-format file\n";
	std::cerr << "  zlib-check - decompresses a ZLIB-format file, discarding the contents\n";
	std::cerr << "  nbt-to-xml - converts an NBT file to an equivalent XML file\n";
	std::cerr << "  nbt-from-xml - converts an NBT-equivalent XML file to an NBT file\n";
	std::cerr << "  nbt-block-substitute - replaces block IDs in the terrain of an NBT file\n";
	std::cerr << "  nbt-patch-barray - replaces specific byte values in NBT byte arrays with other values\n";
}

int main_impl(int argc, char **argv) {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

	// Wrap the command-line parameters in a view.
	std::ranges::subrange<char **> args(argv, argv + argc);

	// Extract the application name.
	appname = args.front();
	args.advance(1);

	// Extract the command name.
	if(args.empty()) {
		usage();
		return 1;
	}
	std::string_view command = args.front();
	args.advance(1);

	// Dispatch.
	if(command == "coord-calc") {
		return CoordCalc::calc(args);
	} else if(command == "region-unpack") {
		return Region::unpack(args);
	} else if(command == "region-pack") {
		return Region::pack(args);
	} else if(command == "zlib-decompress") {
		return ZLib::decompress(args);
	} else if(command == "zlib-compress") {
		return ZLib::compress(args);
	} else if(command == "zlib-check") {
		return ZLib::check(args);
	} else if(command == "nbt-to-xml") {
		return NBT::to_xml(args);
	} else if(command == "nbt-from-xml") {
		return NBT::from_xml(args);
	} else if(command == "nbt-block-substitute") {
		return NBT::block_substitute(args);
	} else if(command == "nbt-patch-barray") {
		return NBT::patch_barray(args);
	} else {
		usage();
	}

	return 0;
}
}

/**
 * \brief The application entry point.
 *
 * \param[in] argc the number of command-line arguments, including the
 * application name
 *
 * \param[in] argv the command-line arguments
 */
int main(int argc, char **argv) {
	try {
		return main_impl(argc, argv);
	} catch(const Glib::Exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch(const std::exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch(...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}
