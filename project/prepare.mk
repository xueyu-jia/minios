$(call import,lib.core lib.io lib.string)

NESTED_MAKE := $(MAKE) ENABLE_BUILD_LOG=

CACHE_DIR      := $(OBJDIR).cache/
ARCH_CACHE_DIR := $(CACHE_DIR)$(ARCH)/
ARCH_FLAGS_DIR := $(ARCH_CACHE_DIR).flags/

#! ATTENTION: user depends on source_files and mapping
#! FIXME: unwanted internal dependencies
PREPARE_ORDERED_SEQ := toolchain source_files mapping user generate deps fstools image gdb qemu
$(call import,$(addprefix prepare.,$(PREPARE_ORDERED_SEQ)))
