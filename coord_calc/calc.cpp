#include "coord_calc/coord_calc.h"
#include "util/globals.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace mcwutil::coord_calc {
namespace {
int divfloor(int num, int den) {
	return (num < 0 ? num - (den - 1) : num) / den;
}

int real_mod(int num, int den) {
	num %= den;
	if(num < 0) {
		num += den;
	}
	return num;
}

/**
 * \brief Converts a user-provided string to an integer.
 *
 * \param[in] s the string to parse.
 *
 * \return the integer value.
 */
int parse_int(const std::string &s) {
	std::size_t consumed;
	int value = std::stoi(s, &consumed);
	if(consumed != s.size()) {
		throw std::invalid_argument("non-integer characters following integer");
	}
	return value;
}

void usage() {
	std::cerr << "Usage:\n";
	std::cerr << appname << " coord-calc X Z\n";
	std::cerr << '\n';
	std::cerr << "Calculates chunk numbers, region numbers, and chunk offsets from coordinate pairs.\n";
	std::cerr << '\n';
	std::cerr << "Arguments:\n";
	std::cerr << "  X - the integer floor of the X coordinate of the point\n";
	std::cerr << "  Z - the integer floor of the Y coordinate of the point\n";
}
}
}

/**
 * \brief Entry point for the \c coord-calc utility.
 *
 * \param[in] args the command-line arguments
 */
int mcwutil::coord_calc::calc(std::ranges::subrange<char **> args) {
	// Check and parse parameters.
	if(args.size() != 2) {
		usage();
		return 1;
	}
	int x, z;
	try {
		x = parse_int(args[0]);
		z = parse_int(args[1]);
	} catch(const std::invalid_argument &) {
		usage();
		return 1;
	}

	// Display results.
	std::cout << "The following information pertains to the column of blocks between X=" << x << " and X=" << (x + 1) << " and between Z=" << z << " and Z=" << (z + 1) << ", centred at (" << (x + 0.5) << ", " << (z + 0.5) << ").\n";
	int chunkx = divfloor(x, 16), chunkz = divfloor(z, 16);
	std::cout << "This location is contained within the global chunk (" << chunkx << ", " << chunkz << ").\n";
	int regionx = divfloor(chunkx, 32), regionz = divfloor(chunkz, 32);
	std::cout << "This chunk is contained within region file r." << regionx << '.' << regionz << ".mca.\n";
	std::cout << "This region file contains the data between X=" << (regionx * 32 * 16) << " and X=" << ((regionx + 1) * 32 * 16) << " and between Z=" << (regionz * 32 * 16) << " and Z=" << ((regionz + 1) * 32 * 16) << ".\n";
	int offset = real_mod(chunkx, 32) + 32 * real_mod(chunkz, 32);
	std::cout << "The pointer to the chunk data is found at index " << offset << " within the pointer array in the anvil file header.\n";
	return 0;
}
