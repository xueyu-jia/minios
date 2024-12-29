$(COMPILE_DB_FILE): $(TOOL_OBJDIR)lightbear force
	@mkdir -p $(@D)
	@$(call enter-job,dump,$(notdir $@))
	@$(NESTED_MAKE) $(__ARGS__) -s -B -n $(ERR2NUL) | grep -P '^($(HOST_CC)|$(ARCH_CC)) .*$$' | $< > $@
	@$(call leave-job,done,dump,$(notdir $@))
