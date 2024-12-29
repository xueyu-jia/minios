$(call import,lib.core lib.string)

icase_var_name = $(call map_or_else,is_defined,$(call to_upper,$(1)),$(call map_or_else,is_defined,$(call to_lower,$(1)),))
icase_var = $(call or_else,$($(call to_upper,$(1))),$($(call to_lower,$(1))))
export_icase_var = $(if $(call icase_var_name,$(1)),$(eval $(1) = $(call icase_var,$(1))),)
