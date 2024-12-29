$(call import,lib.io)

find_config_in_recursive = $(call find_files_recursive,./$(1),in,)
config_in2gen = $(patsubst %.in,$(CONFIG_OBJDIR)%,$(1))

CONFIG_IN_FILES  := $(call find_config_in_recursive,)
CONFIG_GEN_FILES := $(call config_in2gen,$(CONFIG_IN_FILES))

ENVS_FILE := $(CACHE_DIR).envs
