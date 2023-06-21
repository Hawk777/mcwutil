# ========== STOP ========== STOP ========== STOP ==========
#
# The contents of this file should not generally be modified.
# The rules in this file are generic and compatible with building multiple executables.
# To change the set of executables compiled, the set sources for each executable, or the compilation or linking flags, edit project.mk instead.
#
# ========== STOP ========== STOP ========== STOP ==========

#
# Some rules will need second expansions as they are static pattern rules which do variable dereferences based on target name.
#
.SECONDEXPANSION :

#
# Pull in the project rules.
#
include project.mk

#
# Build a full CXXFLAGS, LDFLAGS, and LIBS.
#
PKG_CONFIG ?= pkg-config
override CXXFLAGS := $(PROJECT_CXXFLAGS) $(shell $(PKG_CONFIG) --cflags $(PACKAGES) | sed 's/-I/-isystem /g') $(CXXFLAGS)
override LDFLAGS := $(PROJECT_LDFLAGS) $(shell $(PKG_CONFIG) --libs-only-other --libs-only-L $(PACKAGES)) $(LDFLAGS)
override LIBS := $(PROJECT_LIBS) $(shell $(PKG_CONFIG) --libs-only-l $(PACKAGES)) $(LIBS)

#
# The target to build everything.
#
.PHONY : world
.DEFAULT_GOAL := world
world : $(addprefix bin/,$(filter-out $(EXECUTABLES_EXCLUDE_WORLD),$(EXECUTABLES)))

#
# Prevent echoing of compilation commands.
#
.SILENT :

#
# Delete output files if the rule trying to build them fails.
#
.DELETE_ON_ERROR:

#
# Rule to build the documentation.
#
.PHONY : doc
doc :
	doxygen

#
# Rule to build a cross-reference database.
#
.PHONY : tags
tags :
	ctags -R

#
# Rule to link a final executable.
#
$(addprefix bin/,$(EXECUTABLES)) : bin/% : $$(sort $$(addprefix obj/,$$(subst .cpp,.o,$$(shell find $$(SOURCES_%) -path ./.git -prune -o -name '*.cpp' -print))))
	echo "  LD	$@"
	mkdir -p bin
	$(CXX) $(LDFLAGS) $(if $(PACKAGES_$(notdir $@)),$(shell $(PKG_CONFIG) --libs-only-other --libs-only-L $(PACKAGES_$(notdir $@)))) -o$@ $+ $(LIBS) $(if $(PACKAGES_$(notdir $@)),$(shell $(PKG_CONFIG) --libs-only-l $(PACKAGES_$(notdir $@))))

#
# Rule to make a .d file and a .o file from a .cpp file.
#
# As advised on <http://make.paulandlesley.org/autodep.html>, the .d file is not included in the list of targets.
# This is acceptable, because:
# (1) when the .d and .o files do not exist, they will be built because the nonexistent .o file is out of date,
# (2) when a dependency is updated, both the .d and .o files will be rebuilt because the old .o file is out of date, and
# (3) it's impossible to alter the set of dependencies without modifying the .cpp file or one of its includes, which makes the .o file out of date
#
obj/%.o : %.cpp
	$(RM) $(@:.o=.d) $@
	echo "  CXX   obj/$(<:.cpp=.o)"
	mkdir -p $(dir obj/$(<:.cpp=.o))
	$(CXX) $(CXXFLAGS) -o $@ -c -MT '$@' -MMD -MP $<

#
# Include all the dependency files.
#
ALL_D_FILES=$(addprefix obj/,$(subst ./,,$(subst .cpp,.d,$(shell find . -name .svn -prune -o -name '*.cpp' -print))))
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),tags)
ifneq ($(MAKECMDGOALS),doc)
ifneq ($(MAKECMDGOALS),uncrustify)
-include $(ALL_D_FILES)
endif
endif
endif
endif

#
# Rule to clean generated files.
#
.PHONY : clean
clean :
	$(RM) -rf bin html obj
	$(RM) -f tags

#
# Rule to check formatting of source files.
#
.PHONY : check-format
check-format :
	clang-format --Werror --dry-run --style=file $(shell find $(SOURCES_%) -path ./.git -prune -o \( -name '*.cpp' -o -name '*.h' \) -print)
