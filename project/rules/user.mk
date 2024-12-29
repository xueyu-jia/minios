define declare_user_bin_deps
$(1) $(patsubst %.bin,%.dbg,$(1)):         \
	$(ARCH_DIR)user/linker.ld              \
	$(call get_user_bin_deps_objects,$(1)) \
	$(ARCH_USER_OBJECT_FILES) $(LIBULIB_FILE) $(LIBGENERIC_FILE)
user: $(1) $(patsubst %.bin,%.dbg,$(1))
endef
$(foreach bin,$(USER_PROGRAM_FILES),$(eval $(call declare_user_bin_deps,$(bin))))

$(USER_OBJDIR)%.bin:
	@mkdir -p $(dir $@)
	@$(call enter-job,ld,$(notdir $@))
	@$(ARCH_LD) $(ARCH_LD_FLAGS) -s -T $< -o $@ $(wordlist 2,$(words $^),$^) $(ERR2LOG)
	@$(call leave-job,done,ld,$(notdir $@))

$(USER_OBJDIR)%.dbg:
	@mkdir -p $(dir $@)
	@$(call enter-job,ld,$(notdir $@))
	@$(ARCH_LD) $(ARCH_LD_FLAGS) -T $< -o $@ $(wordlist 2,$(words $^),$^) $(ERR2LOG)
	@$(call leave-job,done,ld,$(notdir $@))
