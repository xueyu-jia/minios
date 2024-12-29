$(call import,lib.io)

MERGE_SCRIPT := scripts/mergedep.pl

SOURCE_DEPS_FILE := $(ARCH_CACHE_DIR).deps

define refresh-source-deps-file
	if [ ! -z "$(2)" ]; then                      \
		mkdir -p `dirname $(1)`;                  \
		perl $(MERGE_SCRIPT) $(1).swp $(2);       \
		cmp -s $(1) $(1).swp || cp $(1).swp $(1); \
	fi
endef
$(shell $(call refresh-source-deps-file,$(SOURCE_DEPS_FILE),$(call find_files_recursive,./$(ARCH_OBJDIR),d,)))
