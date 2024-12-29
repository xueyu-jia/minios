$(call import,lib.io)

BOOT_SINGLE_BIN_TARGETS := mbr boot

BOOT_SOURCES := $(filter $(ARCH_DIR)boot/%,$(MINIOS_SOURCES))

#! boot output files compiled from single assembly source, including single-sourced loader files
BOOT_SINGLE_SOURCES := $(shell $(ECHO) $(foreach type,$(BOOT_SINGLE_BIN_TARGETS),$(foreach src,$(BOOT_SOURCES),$(filter %/$(type).asm,$(src)))))

#! loader output files compiled from multiple sources
BOOT_LOADER_SOURCES := $(foreach src,$(BOOT_SOURCES),$(if $(call contains,$(BOOT_SINGLE_SOURCES),$(src)),,$(src)))

#! a loader without link script is assumed to be a single-sourced loader
define filter_single_sourced_loader_sources
	$(foreach type,$(SUPPORT_BOOT_TYPES),$(if $(call is_file,$(ARCH_DIR)boot/$(type)/linker.ld),,$(filter $(ARCH_DIR)boot/$(type)/%.asm,$(1))))
endef

BOOT_SINGLE_SOURCES += $(call filter_single_sourced_loader_sources,$(BOOT_LOADER_SOURCES))
BOOT_SINGLE_SOURCES := $(call format_list,$(BOOT_SINGLE_SOURCES))

BOOT_LOADER_SOURCES := $(foreach src,$(BOOT_SOURCES),$(if $(call contains,$(BOOT_SINGLE_SOURCES),$(src)),,$(src)))
BOOT_LOADER_SOURCES := $(call format_list,$(BOOT_LOADER_SOURCES))

define declare_boot_bin_deps
$(call boot_s2b,$(1)): $(call kernel_s2o,$(1))
boot: $(call boot_s2b,$(1))
endef
$(foreach src,$(BOOT_SINGLE_SOURCES),$(eval $(call declare_boot_bin_deps,$(src))))

$(call kernel_s2o,$(BOOT_SINGLE_SOURCES)):
	@mkdir -p $(dir $@)
	@$(call enter-job,as,$<)
	@$(call get_arch_as,BOOT) -MD $(patsubst %.obj,%.d,$@) -o $@ $< $(ALL2LOG)
	@$(call leave-job,done,as,$<)

$(call boot_s2b,$(BOOT_SINGLE_SOURCES)):
	@mkdir -p $(dir $@)
	@$(call enter-job,ld,$(notdir $@))
	@cp -f $< $@
	@$(call leave-job,done,ld,$(notdir $@))

define declare_loader_bin_deps
$(call boot_s2b,$(1)) $(patsubst %.bin,%.dbg,$(call boot_s2b,$(1))):          \
	$(ARCH_DIR)boot/$(2)/linker.ld                                            \
	$(filter $(BOOT_OBJDIR)$(2)/%,$(call kernel_s2o,$(BOOT_LOADER_SOURCES)))  \
	$(LIBKLIB_FILE) $(LIBGENERIC_FILE)
boot: $(call boot_s2b,$(1)) $(patsubst %.bin,%.dbg,$(call boot_s2b,$(1)))
endef
$(foreach src,$(filter %/loader.asm,$(BOOT_LOADER_SOURCES)),$(eval $(call declare_loader_bin_deps,$(src),$(call boot_extract_type_from_source,$(src)))))

$(call boot_s2b,$(filter %/loader.asm,$(BOOT_LOADER_SOURCES))):
	@mkdir -p $(dir $@)
	@$(call enter-job,ld,$(notdir $@))
	@$(ARCH_LD) $(ARCH_LD_FLAGS) -s --oformat binary -T $< -o $@ $(wordlist 2,$(words $^),$^) $(ERR2LOG)
	@$(call leave-job,done,ld,$(notdir $@))

$(patsubst %.bin,%.dbg,$(call boot_s2b,$(filter %/loader.asm,$(BOOT_LOADER_SOURCES)))):
	@mkdir -p $(dir $@)
	@$(call enter-job,ld,$(notdir $@))
	@$(ARCH_LD) $(ARCH_LD_FLAGS) -T $< -o $@ $(wordlist 2,$(words $^),$^) $(ERR2LOG)
	@$(call leave-job,done,ld,$(notdir $@))
