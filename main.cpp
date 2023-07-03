#include "calc.h"
#include "nbt/nbt.h"
#include "region/region.h"
#include "util/globals.h"
#include "util/xml.h"
#include "zlib_utils.h"
#include <exception>
#include <iostream>
#include <locale>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <typeinfo>

namespace mcwutil {
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
		return calc::coord(args);
	} else if(command == "region-unpack") {
		return region::unpack(args);
	} else if(command == "region-pack") {
		return region::pack(args);
	} else if(command == "zlib-decompress") {
		return zlib::decompress(args);
	} else if(command == "zlib-compress") {
		return zlib::compress(args);
	} else if(command == "zlib-check") {
		return zlib::check(args);
	} else if(command == "nbt-to-xml") {
		return nbt::to_xml(args);
	} else if(command == "nbt-from-xml") {
		return nbt::from_xml(args);
	} else if(command == "nbt-block-substitute") {
		return nbt::block_substitute(args);
	} else if(command == "nbt-patch-barray") {
		return nbt::patch_barray(args);
	} else {
		usage();
	}

	return 0;
}
}
}

/**
 * \brief The application entry point.
 *
 * \param[in] argc the number of command-line arguments, including the
 * application name.
 *
 * \param[in] argv the command-line arguments.
 *
 * \return the application exit code.
 */
int main(int argc, char **argv) {
	try {
		return mcwutil::main_impl(argc, argv);
	} catch(const std::exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch(const mcwutil::xml::error &exp) {
		std::cerr << typeid(exp).name() << ":\n";
		for(const auto &i : exp.errors) {
			struct {
				void operator()(const std::string &i) {
					std::cerr << "  " << i << '\n';
				}
				void operator()(const xmlError &i) {
					std::cerr << "  ";
					if(i.file) {
						std::cerr << i.file << ':';
						if(!i.line) {
							std::cerr << ' ';
						}
					}
					if(i.line) {
						std::cerr << i.line << ": ";
					}
					std::cerr << i.message;
				}
			} vis;
			std::visit(vis, i);
		}
	} catch(...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}
