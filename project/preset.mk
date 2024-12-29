$(call import,lib.core lib.string)
$(call import,config.consts)

$(call ensures_not_null,$(PRESET),preset is null)

PRESET_MATCH_RESULT := $(shell grep -Pn '\[preset:$(PRESET)\]' $(MINIOS_PRESET_FILE))
$(call ensures_not_null,$(PRESET_MATCH_RESULT),preset $(PRESET) not found)

PRESET_CONF_LIMIT := $(shell awk 'END { print NR }' $(MINIOS_PRESET_FILE))
PRESET_CONF_LINES := $(shell grep -Pn '^\[' $(MINIOS_PRESET_FILE) | sed -re 's/^([0-9]+).*$$/\1/g') $(call arith_inc,$(PRESET_CONF_LIMIT))
PRESET_CONF_START := $(shell $(ECHO) '$(PRESET_MATCH_RESULT)' | sed -re 's/^([0-9]+):.*$$/\1/g')
PRESET_CONF_END   := $(shell $(ECHO) ' $(PRESET_CONF_LINES)' | grep -Po '(?<= $(PRESET_CONF_START) )([0-9]+)')

#! NOTE: the wanted make variable name will be prefixed with $$ to avoid misrecognition

PRESET_CONF := $(shell \
preset='$(MINIOS_PRESET_FILE)';                                                           \
slice_pat='$(call arith_inc,$(PRESET_CONF_START)),$(call arith_dec,$(PRESET_CONF_END))p'; \
fmt_pat='s/^([a-zA-Z0-9_-]+)=([^ ]?)/$$$$\1 = \2/g';                                      \
sed -n "$$slice_pat" "$$preset" | sed -re "$$fmt_pat";                                    \
)

__preset_conf_iname := 0
__preset_conf_vbeg  := 0
__preset_conf_vend  := 0
__preset_conf_dummy := $(PRESET_CONF) $$$$DUMMY =

__preset_conf_is_eq = $(call scmp_eq,=,$(word $(1),$(__preset_conf_dummy)))
__preset_conf_is_var_name = $(call regex_match,^\$$\$$[a-zA-Z0-9_-]+$$,$(call safe_word,$(call arith_dec,$(1)),$(__preset_conf_dummy)))
__preset_conf_is_var_assignment = $(call and,$(call __preset_conf_is_eq,$(1)),$(call __preset_conf_is_var_name,$(1)))
__preset_conf_assignment_available = $(call cmp_ne,0,$(__preset_conf_iname))
__preset_conf_current_var_name = $(call regex_search,(?<=\$$\$$)[a-zA-Z0-9_-]+,$(call safe_word,$(__preset_conf_iname),$(__preset_conf_dummy)))
__preset_conf_current_values = $(wordlist $(__preset_conf_vbeg),$(call arith_dec,$(__preset_conf_vend)),$(__preset_conf_dummy))
__preset_conf_export_current_var = $(eval $(call __preset_conf_current_var_name) = $(call __preset_conf_current_values))

define __preset_conf_on_new_item
	$(eval __preset_conf_vend = $(call arith_dec,$(1)))
	$(if $(call __preset_conf_assignment_available),$(call __preset_conf_export_current_var),)
	$(eval __preset_conf_iname = $(call arith_dec,$(1)))
	$(eval __preset_conf_vbeg = $(call arith_inc,$(1)))
endef

$(foreach index,$(shell seq $(words $(__preset_conf_dummy))),$(if $(call __preset_conf_is_var_assignment,$(index)),$(call __preset_conf_on_new_item,$(index)),))
