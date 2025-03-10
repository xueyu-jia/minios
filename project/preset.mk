$(call import,lib.core lib.string)
$(call import,config.consts)

preset_get_limit = $(shell awk 'END { print NR }' $(MINIOS_PRESET_FILE))
preset_get_lines = $(shell grep -Pn '^\[' $(MINIOS_PRESET_FILE) | sed -re 's/^([0-9]+).*$$/\1/g') $(call arith_inc,$(call preset_get_limit))

# preset_match <preset-name> <include-hidden>
preset_match = $(shell grep -Pn '\[$(if $(2),!?,)preset:$(1)(:inherit\(.*\))?\]' $(MINIOS_PRESET_FILE))
# preset_get_direct_inherit <match-result>
preset_get_direct_inherit = $(shell $(ECHO) '$(1)' | grep -Po '(?<=:inherit\()(.*)(?=\))' | sed 's/,/ /g')

# _preset_get_inherit_list_impl <varno> <start> <self>
define _preset_get_inherit_list_impl
	$(eval $(1)_end = $(call arith_inc,$(words $($(1)_inherit_list))))
	$(foreach index,$(call range,$(2),$($(1)_end)),                                                        \
		$(eval $(1)_preset = $(word $(index),$($(1)_inherit_list)))                                        \
		$(eval $(1)_sub_inherit = $(call preset_get_direct_inherit,$(call preset_match,$($(1)_preset),1))) \
		$(eval $(1)_inherit_list = $($(1)_inherit_list) $($(1)_sub_inherit)))
	$(eval $(1)_inherit_list = $(call uniq,$($(1)_inherit_list)))
	$(if $(call cmp_lt,$($(1)_end),$(words $($(1)_inherit_list))),$(call $(3),$(1),$($(1)_end),$(3)))
endef
# preset_get_inherit_list <preset>
_preset_get_inherit_list = $(foreach varno,$(call new_uuid),                       \
	$(eval $(varno)_inherit_list = $(1))                                           \
	$(call _preset_get_inherit_list_impl,$(varno),1,_preset_get_inherit_list_impl) \
	$(wordlist 2,$(words $($(varno)_inherit_list)),$($(varno)_inherit_list)))
preset_get_inherit_list = $(call format_list,$(call _preset_get_inherit_list,$(1)))

# _preset_locate_start <match-result>
_preset_locate_start = $(shell $(ECHO) '$(1)' | sed -re 's/^([0-9]+):.*$$/\1/g')
# _preset_locate_end <start>
_preset_locate_end = $(shell $(ECHO) ' $(call preset_get_lines) ' | grep -Po '(?<= $(1) )([0-9]+)')
# _preset_read_conf <start> <end>
_preset_read_conf = $(shell \
preset='$(MINIOS_PRESET_FILE)';                             \
slice_pat='$(call arith_inc,$(1)),$(call arith_dec,$(2))p'; \
fmt_pat='s/^([a-zA-Z0-9_-]+)=([^ ]?)/@\1@ = \2/g';          \
sed -n "$$slice_pat" "$$preset" | sed -re "$$fmt_pat";      \
)

# _preset_make_dummy_conf <match-result>
_preset_make_dummy_conf_wrapper = $(call _preset_read_conf,$(1),$(call _preset_locate_end,$(1))) @DUMMY@ =
_preset_make_dummy_conf = $(call _preset_make_dummy_conf_wrapper,$(call _preset_locate_start,$(1)))

# _preset_conf_is_eq <index> <conf-list-name>
_preset_conf_is_eq = $(call scmp_eq,=,$(word $(1),$($(2))))
# _preset_conf_is_var_name <index> <conf-list-name>
_preset_conf_is_var_name = $(call regex_match,^@[a-zA-Z0-9_-]+@$$,$(call safe_word,$(call arith_dec,$(1)),$($(2))))
# _preset_conf_is_var_assignment <index> <conf-list-name>
_preset_conf_is_var_assignment = $(call and,$(call _preset_conf_is_eq,$(1),$(2)),$(call _preset_conf_is_var_name,$(1),$(2)))
# _preset_conf_assignment_available <iname>
_preset_conf_assignment_available = $(call cmp_ne,0,$(1))
# _preset_conf_current_var_name <iname> <conf-list-name>
_preset_conf_current_var_name = $(call regex_search,(?<=@)[a-zA-Z0-9_-]+(?=@),$(call safe_word,$(1),$($(2))))
# _preset_conf_current_values <vbeg> <vend> <conf-list-name>
_preset_conf_current_values = $(wordlist $(1),$(call arith_dec,$(2)),$($(3)))
# _preset_conf_export_current_var <iname> <vbeg> <vend> <conf-list-name>
_preset_conf_export_current_var = $(eval $(call _preset_conf_current_var_name,$(1),$(4)) = $(call _preset_conf_current_values,$(2),$(3),$(4)))
# _preset_conf_on_new_item <varno> <index> <conf-list-name>
define _preset_conf_on_new_item
	$(eval $(1)_vend = $(call arith_dec,$(2)))
	$(if $(call _preset_conf_assignment_available,$($(1)_iname)), \
		$(call _preset_conf_export_current_var,$($(1)_iname),$($(1)_vbeg),$($(1)_vend),$(3)),)
	$(eval $(1)_iname = $(call arith_dec,$(2)))
	$(eval $(1)_vbeg = $(call arith_inc,$(2)))
endef

# _preset_conf_load_direct <varno> <match-result>
define _preset_conf_load_direct
	$(eval $(1)_iname = 0)
	$(eval $(1)_vbeg = 0)
	$(eval $(1)_vend = 0)
	$(eval $(1)_dummy_list = $(call _preset_make_dummy_conf,$(2)))
	$(foreach index,$(shell seq $(words $($(1)_dummy_list))),                 \
		$(if $(call _preset_conf_is_var_assignment,$(index),$(1)_dummy_list), \
			$(call _preset_conf_on_new_item,$(1),$(index),$(1)_dummy_list),))
endef
# _preset_conf_load <varno> <preset>
define _preset_conf_load
	$(eval $(1)_presets = $(2) $(call preset_get_inherit_list,$(2)))
	$(foreach index,$(call range,$(words $($(1)_presets)),0,-1),                     \
		$(eval $(1)_match = $(call preset_match,$(word $(index),$($(1)_presets)),1)) \
		$(if $($(1)_match),$(call _preset_conf_load_direct,$(1),$($(1)_match))))
endef
# preset_conf_load <preset>
preset_conf_load = $(foreach varno,$(call new_uuid),$(call _preset_conf_load,$(varno),$(1)))

$(call ensures_not_null,$(PRESET),preset is null)
$(call ensures_not_null,$(call preset_match,$(PRESET),),failed to load preset $(PRESET))

$(call preset_conf_load,$(PRESET))
