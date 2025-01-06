$(call import,lib.io)

TOOL_PROGRAM_FILES := $(foreach t,$(wildcard tools/*),$(patsubst tools/%,$(TOOL_OBJDIR)%,$(call map_or_else,is_dir,$(t),)))

define declare_tool_bin_deps
$(TOOL_OBJDIR)$(1): $(filter tools/$(1)/%,$(TOOL_SOURCES))
endef
$(foreach t,$(TOOL_PROGRAM_FILES),$(eval $(call declare_tool_bin_deps,$(patsubst $(TOOL_OBJDIR)%,%,$(t)))))

$(TOOL_PROGRAM_FILES):
	@mkdir -p $(dir $@)
	@$(call enter-job,mktool,$(notdir $@))
	@$(HOST_CC) $(HOST_CC_FLAGS) -I$(dir $@) -o $@ $^
	@$(call leave-job,done,mktool,$(notdir $@))
