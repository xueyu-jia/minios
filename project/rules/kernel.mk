KERNEL_SOURCES := $(filter-out $(ARCH_DIR)boot/%,$(filter-out klib/%,$(MINIOS_SOURCES)))

$(KERNEL_FILE) $(KERNEL_DEBUG_FILE): $(call kernel_s2o,$(KERNEL_SOURCES)) $(LIBKLIB_FILE) $(LIBGENERIC_FILE)

$(KERNEL_FILE):
	@mkdir -p $(dir $@)
	@$(call enter-job,ld,$(notdir $@))
	@$(ARCH_LD) $(ARCH_LD_FLAGS) -s -Ttext $(KERNEL_START_ADDR) -o $@ $^ $(ERR2LOG)
	@$(call leave-job,done,ld,$(notdir $@))

$(KERNEL_DEBUG_FILE):
	@mkdir -p $(dir $@)
	@$(call enter-job,ld,$(notdir $@))
	@$(ARCH_LD) $(ARCH_LD_FLAGS) -Ttext $(KERNEL_START_ADDR) -o $@ $^ $(ERR2LOG)
	@$(call leave-job,done,ld,$(notdir $@))
