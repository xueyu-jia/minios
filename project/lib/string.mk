$(call import,lib.core)

COMMA = ,

quoted = "$(1)"
squoted = '$(1)'

slice = $(shell raw="$(1)"; parts=($$raw); $(ECHO) $${parts[@]:$(2):$(3)})

join_parts = $(shell raw="$(1)"; $(ECHO) $${raw// /$(2)})

filter_csi = $(shell $(ECHO) "$1" | sed -r 's/\\e\[((\?[0-9]+[hl])|([0-9]?(;[0-9]+)*m))//g')

regex_search = $(shell $(ECHO) '$(2)' | grep -Po '$(1)')
regex_match = $(call is_not_null,$(call regex_search,$(1),$(2)))

is_hex = $(call regex_match,^0[xX][0-9a-fA-F]+$$,$(1))

to_lower = $(shell $(ECHO) '$(1)' | tr '[:upper:]' '[:lower:]')
to_upper = $(shell $(ECHO) '$(1)' | tr '[:lower:]' '[:upper:]')

scmp_eq = $(call test,'$(1)' == '$(2)')
scmp_ne = $(call test,'$(1)' != '$(2)')
