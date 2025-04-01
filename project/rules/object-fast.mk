define declare_generic_obj_deps
$(call generic_s2o,$(1)): $(1)
endef
$(foreach src,$(GENERIC_SOURCES),$(eval $(call declare_generic_obj_deps,$(src))))

define declare_kernel_obj_deps
$(call kernel_s2o,$(1)): $(1)
endef
$(foreach src,$(MINIOS_SOURCES),$(eval $(call declare_kernel_obj_deps,$(src))))

define declare_user_obj_deps
$(call user_s2o,$(1)): $(1)
endef
$(foreach src,$(USER_SOURCES),$(eval $(call declare_user_obj_deps,$(src))))

$(GENERIC_OBJDIR)%.c.obj:
	@mkdir -p $(dir $@)
	@$(call enter-job,cc,$<)
	@$(call get_arch_cc,GENERIC) -MD -c -o $@ -x c $< $(ERR2LOG)
	@$(call leave-job,done,cc,$<)

$(BOOT_OBJDIR)%.c.obj:
	@mkdir -p $(dir $@)
	@$(call enter-job,cc,$<)
	@$(call get_arch_cc,BOOT) -MD -c -o $@ -x c $< $(ERR2LOG)
	@$(call leave-job,done,cc,$<)

$(BOOT_OBJDIR)%.asm.obj:
	@mkdir -p $(dir $@)
	@$(call enter-job,as,$<)
	@$(call get_arch_as,BOOT) -MD $(patsubst %.obj,%.d,$@) -f elf -o $@ $< $(ERR2LOG)
	@$(call leave-job,done,as,$<)

$(KERNEL_OBJDIR)%.c.obj:
	@mkdir -p $(dir $@)
	@$(call enter-job,cc,$<)
	@$(call get_arch_cc,KERNEL) -MD -c -o $@ -x c $< $(ERR2LOG)
	@$(call leave-job,done,cc,$<)

$(KERNEL_OBJDIR)%.asm.obj:
	@mkdir -p $(dir $@)
	@$(call enter-job,as,$<)
	@$(call get_arch_as,KERNEL) -MD $(patsubst %.obj,%.d,$@) -f elf -o $@ $< $(ERR2LOG)
	@$(call leave-job,done,as,$<)

$(ULIB_OBJDIR)%.c.obj:
	@mkdir -p $(dir $@)
	@$(call enter-job,cc,$<)
	@$(call get_arch_cc,ULIB) -MD -c -o $@ -x c $< $(ERR2LOG)
	@$(call leave-job,done,cc,$<)

$(ULIB_OBJDIR)%.asm.obj:
	@mkdir -p $(dir $@)
	@$(call enter-job,as,$<)
	@$(call get_arch_as,ULIB) -MD $(patsubst %.obj,%.d,$@) -f elf -o $@ $< $(ERR2LOG)
	@$(call leave-job,done,as,$<)

$(USER_OBJDIR)%.c.obj:
	@mkdir -p $(dir $@)
	@$(call enter-job,cc,$<)
	@$(call get_arch_cc,USER) -MD -w -c -o $@ -x c $< $(ERR2LOG)
	@$(call leave-job,done,cc,$<)

$(USER_OBJDIR)%.asm.obj:
	@mkdir -p $(dir $@)
	@$(call enter-job,as,$<)
	@$(call get_arch_as,USER) -MD $(patsubst %.obj,%.d,$@) -f elf -o $@ $< $(ERR2LOG)
	@$(call leave-job,done,as,$<)
