$(call import,config.consts)
$(call import,lib.core lib.io lib.string)

is_machine_hex = $(call regex_match,^0[xX][0-9a-fA-F]{1$(COMMA)16}$$,$(1))

arch_list_boot_dirs = $(foreach d,$(wildcard arch/$(1)/boot/*),$(if $(call is_dir,$(d)),$(d)))
arch_list_provided_boot_types = $(filter-out include,$(notdir $(call arch_list_boot_dirs,$(1))))
arch_get_boot_types = $(foreach t,$(call arch_list_provided_boot_types,$(1)),$(if $(call contains,$(SUPPORT_BOOT_TYPES),$(t)),$(t),))
