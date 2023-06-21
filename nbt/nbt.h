#ifndef NBT_NBT_H
#define NBT_NBT_H

#include <string>
#include <vector>

namespace NBT {
int to_xml(const std::vector<std::string> &args);
int from_xml(const std::vector<std::string> &args);
int block_substitute(const std::vector<std::string> &args);
int patch_barray(const std::vector<std::string> &args);
}

#endif
