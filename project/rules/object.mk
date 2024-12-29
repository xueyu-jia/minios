$(call import,lib.core)

#! category = <scope>.<object-dir>
#! scope    = <source-set>.<mapping-rule>
category_get_source_set   = $(foreach category,$(1),$(word 1,$(subst ., ,$(category))))
category_get_mapping_rule = $(foreach category,$(1),$(word 2,$(subst ., ,$(category))))
category_get_object_dir   = $(foreach category,$(1),$(word 3,$(subst ., ,$(category))))
category_get_scope        = $(join $(call category_get_source_set,$(1)),$(addprefix .,$(call category_get_mapping_rule,$(1))))

lookup_object_by_category = $(call $(call category_get_mapping_rule,$(1))_s2o,$(2))
lookup_source_by_category = $(call $(call category_get_mapping_rule,$(1))_o2s,$(2))
lookup_object_by_scope    = $(call lookup_object_by_category,$(1)._,$(2))
lookup_source_by_scope    = $(call lookup_source_by_category,$(1)._,$(2))

OBEJCT_CATEGORIES := GENERIC.generic.GENERIC MINIOS.kernel.BOOT MINIOS.kernel.KERNEL USER.user.ULIB USER.user.USER
OBEJCT_SCOPES := $(sort $(call category_get_scope,$(OBEJCT_CATEGORIES)))
OBJECT_FILES  := $(foreach scope,$(OBEJCT_SCOPES),$(call lookup_object_by_scope,$(scope),$(call safe_deref,$(word 1,$(subst ., ,$(scope)))_SOURCES)))

#! ensures source of <object> in <source-set>
_get_object_category_from_dir__category_test_scope = $(filter $(call $(2)_o2s,$(3)),$(call safe_deref,$(1)_SOURCES))
#! ensures <object> is under <object-dir> or its corresponding arch-dependent dir
_get_object_category_from_dir__category_test_category = $(foreach dir,$(call safe_deref,$(2)_OBJDIR), \
	$(filter $(dir)%,$(3)) $(if $(filter USER,$(1)),$(filter $(patsubst $(ARCH_OBJDIR)%,$(ARCH_OBJDIR)arch/%,$(dir))%,$(3)),))
_get_object_category_from_dir__category_test = $(call and,             \
	$(call _get_object_category_from_dir__category_test_scope,         \
		$(word 1,$(subst ., ,$(1))),$(word 2,$(subst ., ,$(1))),$(2)), \
	$(call _get_object_category_from_dir__category_test_category,      \
		$(word 1,$(subst ., ,$(1))),$(word 3,$(subst ., ,$(1))),$(2)))
_get_object_category_from_dir_impl = $(foreach category,$(OBEJCT_CATEGORIES),        \
	$(if $(call is_defined,_$(1)_found),,                                            \
		$(if $(call _get_object_category_from_dir__category_test,$(category),$(2),), \
			$(eval _$(1)_result := $(category))                                      \
			$(eval _$(1)_found := 1),)))
_get_object_category_from_dir_wrapper = $(foreach varno,$(call new_uuid), \
	$(eval _$(varno)_result := )                                          \
	$(call _get_object_category_from_dir_impl,$(varno),$(1))              \
	$(_$(varno)_result))
get_object_category_from_dir = $(call safe_word,1,$(call format_list,$(call _get_object_category_from_dir_wrapper,$(1))))

define declare_arch_obj_deps
$(1): $(call lookup_source_by_category,$(2),$(1)) $(if $(filter-out GENERIC,$(2)),$(CONFIG_GEN_FILES),) $(ARCH_FLAGS_DIR)$(2)
endef
$(foreach obj,$(OBJECT_FILES),$(eval $(call declare_arch_obj_deps,$(obj),$(call get_object_category_from_dir,$(obj)))))

get_category_arch_cc = $(call get_arch_cc,$(call category_get_object_dir,$(patsubst $(ARCH_FLAGS_DIR)%,%,$(1))))
get_category_arch_as = $(call get_arch_as,$(call category_get_object_dir,$(patsubst $(ARCH_FLAGS_DIR)%,%,$(1))))

$(ARCH_OBJDIR)%.c.obj:
	@mkdir -p $(dir $@)
	@$(call enter-job,cc,$<)
	@$(call get_category_arch_cc,$(lastword $^)) -MD -c -o $@ -x c $< $(ERR2LOG)
	@$(call leave-job,done,cc,$<)

$(ARCH_OBJDIR)%.asm.obj:
	@mkdir -p $(dir $@)
	@$(call enter-job,as,$<)
	@$(call get_category_arch_as,$(lastword $^)) -MD $(patsubst %.obj,%.d,$@) -f elf -o $@ $< $(ERR2LOG)
	@$(call leave-job,done,as,$<)
