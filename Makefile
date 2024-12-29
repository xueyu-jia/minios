#! minios pragma: optional
#! where to find project makefiles
PROJMK_PREFIX := project/

#! minios pragma: optional
#! where to place build artifacts
OBJDIR := build/

#! minios pragma: optional
#! list project modules import progress
TRACE_IMPORTS :=

#! minios pragma: optional
#! rules allowed to bypass the check step
BYPASS_RULES := help clean format fmt list-envs

include $(PROJMK_PREFIX)prelude.mk

BUILD_LOG_FILE  := $(OBJDIR)make.log
COMPILE_DB_FILE := $(OBJDIR)compile_commands.json

$(call import,utils.logger utils.fmt)

$(call import,config.global config.consts)

$(call import,lib.env)
$(call export_icase_var,PRESET)
$(if $(call is_defined,PRESET),$(call import,preset),)

$(call import,config.export)

#! skip check only if all goals are bypassed
ifeq ($(filter $(words $(__GOALS__)),$(words $(foreach bypass,$(BYPASS_RULES),$(filter $(bypass),$(__GOALS__))))),)
$(call import,check)
endif
$(call import,prepare rules)
