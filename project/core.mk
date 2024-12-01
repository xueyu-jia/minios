map = $(foreach v,$(2),$(call $(1),$(v)))

index = $(shell raw="$(2)"; parts=($$raw); $(ECHO) $${parts[$(1)]})

select = $(shell [ $(1) ] && $(ECHO) "$(2)" || $(ECHO) "$(3)")

test = $(call select,$(1),1,)

assert = $(if $(call test,$(1)),,$(error $(2)))
