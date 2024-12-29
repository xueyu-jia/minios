$(call import,lib.core lib.io)
$(call import,config.consts)

find_sources_recursive = $(call find_files_recursive,./$(1),c asm,$(2))

filter_boot_sources = $(foreach src,$(1),$(foreach type,$(SUPPORT_BOOT_TYPES),$(filter $(ARCH_DIR)boot/$(type)/%,$(src))))
filter_valid_kernel_sources = $(filter-out $(ARCH_DIR)boot/%,$(1)) $(call filter_boot_sources,$(filter $(ARCH_DIR)boot/%,$(1)))
filter_kernel_sources = $(call filter_valid_kernel_sources,$(filter-out $(ARCH_DIR)user/%,$(1)))

GENERIC_SOURCES := $(call map,find_sources_recursive,$(LIBGENERIC_SOURCE_DIR))
MINIOS_SOURCES  := $(call filter_kernel_sources,$(call map,find_sources_recursive,$(MINIOS_MODULES)))
USER_SOURCES    := $(call map,find_sources_recursive,$(USER_MODULES))
TOOL_SOURCES    := $(call find_sources_recursive,tools,)
