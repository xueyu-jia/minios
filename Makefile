#! minios pragma: optional
PROJMK_PREFIX := project/

#! minios pragma: optional
OBJDIR := build/

BUILD_LOG_FILE  := $(OBJDIR)make.log
COMPILE_DB_FILE := $(OBJDIR)compile_commands.json

include $(PROJMK_PREFIX)prelude.mk
$(call import,logger)
$(call import,rules)
