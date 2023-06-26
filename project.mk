#
# The executables to compile.
#
EXECUTABLES := mcwutil

#
# The subset of the above that should not be built by the world target.
#
EXECUTABLES_EXCLUDE_WORLD :=

#
# The source files for each executable.
# Directories will be searched recursively for source files.
#
SOURCES_mcwutil := calc.cpp main.cpp nbt region util zlib_utils.cpp

#
# All the pkg-config packages used.
#
PACKAGES := libxml-2.0 zlib

#
# The flags to pass to the linker ahead of any object files.
#
PROJECT_LDFLAGS := -Wl,--as-needed -Wl,-O1 -g

#
# The library flags to pass to the linker after all object files.
#
PROJECT_LIBS :=

#
# The flags to pass to the C++ compiler.
#
PROJECT_CXXFLAGS := -std=gnu++20 -Wall -Wextra -Wformat=2 -Wstrict-aliasing=2 -Wold-style-cast -Wconversion -Wundef -Wmissing-declarations -Wredundant-decls -march=native -O2 -fno-common -fstrict-aliasing -g -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS=1 -DHAVE_INLINE -I.
