#include <mcwutil/calc.hpp>
#include <mcwutil/nbt/nbt.hpp>
#include <mcwutil/region/region.hpp>
#include <mcwutil/util/xml.hpp>
#include <mcwutil/zlib_utils.hpp>
#include <exception>
#include <iostream>
#include <locale>
#include <span>
#include <stdexcept>
#include <string_view>
#include <typeinfo>

namespace mcwutil {
namespace {
/**
 * \brief Displays the usage help text.
 *
 * \param[in] appname The name of the application.
 */
void usage(std::string_view appname) {
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

/**
 * \brief The application entry point, wrapped in exception handling logic.
 *
 * \param[in] argc the number of command-line arguments, including the
 * application name.
 *
 * \param[in] argv the command-line arguments.
 *
 * \return the application exit code.
 */
int main_impl(int argc, char **argv) {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

	// Wrap the command-line parameters in a view.
	std::span<char *> args(argv, argc);

	// Extract the application name.
	std::string_view appname = args.front();
	args = args.subspan(1);

	// Extract the command name.
	if(args.empty()) {
		usage(appname);
		return 1;
	}
	std::string_view command = args.front();
	args = args.subspan(1);

	// Dispatch.
	if(command == "coord-calc") {
		return calc::coord(appname, args);
	} else if(command == "region-unpack") {
		return region::unpack(appname, args);
	} else if(command == "region-pack") {
		return region::pack(appname, args);
	} else if(command == "zlib-decompress") {
		return zlib::decompress(appname, args);
	} else if(command == "zlib-compress") {
		return zlib::compress(appname, args);
	} else if(command == "zlib-check") {
		return zlib::check(appname, args);
	} else if(command == "nbt-to-xml") {
		return nbt::to_xml(appname, args);
	} else if(command == "nbt-from-xml") {
		return nbt::from_xml(appname, args);
	} else if(command == "nbt-block-substitute") {
		return nbt::block_substitute(appname, args);
	} else if(command == "nbt-patch-barray") {
		return nbt::patch_barray(appname, args);
	} else {
		usage(appname);
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
