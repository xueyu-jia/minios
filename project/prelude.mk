ensures_null = $(if $(1),$(error $(2)),)
ensures_not_null = $(if $(1),,$(error $(2)))

ensures_defined = $(if $(filter undefined,$(origin $(1))),$(error undefined required variable: $(1)),)
ensures_undefined = $(if $(filter undefined,$(origin $(1))),,$(error unexpected defined variable: $(1)))

listcmds_defer = which -a $(1) 2> /dev/null
listcmds = $(shell $(call listcmds_defer,$(1)))

define ensures_command
	$(eval $(1) := $(firstword $(call listcmds,$(2))))
	$(call ensures_not_null,$($(1)),command not found: $(2))
endef

define ensures_defined_once
	$(call ensures_null,$($(1)),variable defined multiple times: $(1))
	$(eval $(1) := 1)
endef

$(call ensures_defined,PROJMK_PREFIX)
$(call ensures_defined,OBJDIR)
$(call ensures_defined,TRACE_IMPORTS)

$(call ensures_command,SHELL,bash)
$(call ensures_command,ECHO,echo)

_mod2flag = MINIOS_PROJECT_$(subst .,_,$(1))_MK_IMPORTED
_mod2path = $(subst .,/,$(1))
_strict_import = $(if $(shell [ -f '$(2)' ] && $(ECHO) 1),$(eval include $(2)),$(error import: module $(1) not found))
_import_hook = $(if $(TRACE_IMPORTS),$(info import <$(1)>::$(PROJMK_PREFIX)$(call _mod2path,$(1)).mk),)
define _import
	$(call _import_hook,$(1))
	$(call _strict_import,$(1),$(PROJMK_PREFIX)$(call _mod2path,$(1)).mk)
	$(eval $(call _mod2flag,$(1)) := 1)
endef
import = $(foreach module,$(1),$(if $($(call _mod2flag,$(module))),,$(call _import,$(module))))

#! ATTENTION: __FILE__ is only ensures to be avaiable at the begining of the current makefile context
__FILE__  = $(CURDIR)/$(lastword $(MAKEFILE_LIST))

__DEFAULT_GOAL__ := $(if $(filter undefined,$(origin __DEFAULT_GOAL__)),all,$(__DEFAULT_GOAL__))
__GOALS__ = $(if $(MAKECMDGOALS),$(MAKECMDGOALS),$(if $(__DEFAULT_GOAL__),$(__DEFAULT_GOAL__),<default>))

__ARGS__ = $(shell $(ECHO) $(foreach arg,$(.VARIABLES),$(if $(filter $(origin $(arg)),command line),$(arg)=$($(arg)),)))

ifneq ($(__DEFAULT_GOAL__),)
$(__DEFAULT_GOAL__):
endif

.PHONY: force
