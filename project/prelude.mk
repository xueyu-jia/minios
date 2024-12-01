ensures_null = $(if $(1),$(error $(2)),)
ensures_not_null = $(if $(1),,$(error $(2)))

ensures_defined = $(if $(filter undefined,$(origin $(1))),$(error undefined required variable: $(1)),)
ensures_undefined = $(if $(filter undefined,$(origin $(1))),,$(error unexpected defined variable: $(1)))

define ensures_command
	$(eval $(1) := $(shell which $(2)))
	$(call ensures_not_null,$($(1)),command not found: $(2))
endef

define ensures_defined_once
	$(call ensures_null,$($(1)),variable defined multiple times: $(1))
	$(eval $(1) := 1)
endef

$(call ensures_defined,PROJMK_PREFIX)
$(call ensures_defined,OBJDIR)

_mod2flag = MINIOS_PROJECT_$(subst .,_,$(1))_MK_IMPORTED
define _import
	$(eval include $(PROJMK_PREFIX)$(1).mk)
	$(eval $(call _mod2flag,$(1)) := 1)
endef
import = $(foreach module,$(1),$(if $($(call _mod2flag,$(module))),,$(call _import,$(module))))

# ATTENTION: this only works when each file is only included once, e.g. via `import`
__FILE__ = $(CURDIR)/$(lastword $(MAKEFILE_LIST))

$(call ensures_command,SHELL,bash)
$(call ensures_command,ECHO,echo)
