map = $(foreach v,$(2),$(call $(1),$(v)))

#! or_else <value> <other>
or_else = $(if $(1),$(1),$(2))

#! map_or_else <map-fn> <value> <other>
map_or_else = $(if $(call $(1),$(2)),$(2),$(3))

index = $(shell raw="$(2)"; parts=($$raw); $(ECHO) $${parts[$(1)]})

select = $(shell [ $(1) ] && $(ECHO) "$(2)" || $(ECHO) "$(3)")

test = $(call select,$(1),1,)

assert = $(if $(call test,$(1)),,$(error $(2)))

contains = $(if $(findstring :$(2):,:$(call join_parts,$(1),:):),1,)

not = $(if $(1),,1)
or = $(shell [ "$(1)" ] || [ "$(2)" ] && $(ECHO) 1)
and = $(shell [ "$(1)" ] && [ "$(2)" ] && $(ECHO) 1)

cmp_eq = $(call test,'$(1)' -eq '$(2)')
cmp_ne = $(call test,'$(1)' -ne '$(2)')
cmp_lt = $(call test,'$(1)' -lt '$(2)')
cmp_le = $(call test,'$(1)' -le '$(2)')
cmp_gt = $(call test,'$(1)' -gt '$(2)')
cmp_ge = $(call test,'$(1)' -ge '$(2)')

is_null     = $(call test,-z '$(1)')
is_not_null = $(call not,$(call is_null,$(1)))

is_defined = $(filter-out undefined,$(origin $(1)))
is_not_defined = $(call not,$(call is_defined,$(1)))

arith_bin_op = $(shell $(ECHO) '$(2) $(1) $(3)' | bc)
arith_add = $(call arith_bin_op,+,$(1),$(2))
arith_sub = $(call arith_bin_op,-,$(1),$(2))
arith_mul = $(call arith_bin_op,*,$(1),$(2))
arith_div = $(call arith_bin_op,/,$(1),$(2))
arith_mod = $(call arith_bin_op,%,$(1),$(2))
arith_inc = $(call arith_add,$(1),1)
arith_dec = $(call arith_sub,$(1),1)

format_list = $(shell $(ECHO) $(1))

safe_word = $(word $(if $(call cmp_le,$(1),0),1,$(1)),$(2))
safe_deref = $($(call format_list,$(1)))

new_uuid = $(shell uuidgen | sed 's/-//g')

#! range <n>
#! range <start> <end>
#! range <start> <end> <step>
#! NOTE: <end> is exclusive
_range = $(shell seq $(1) $(2) $(call $(if $(call cmp_lt,$(2),0),arith_inc,arith_dec),$(3)))
range = $(call _range,$(if $(2),$(1),0),$(call or_else,$(3),1),$(call or_else,$(2),$(1)))
