USER_OBJECT_FILES      := $(filter $(USER_OBJDIR)%,$(call user_s2o,$(USER_SOURCES)))
ARCH_USER_OBJECT_FILES := $(filter $(ARCH_OBJDIR)arch/user/%.obj,$(call user_s2o,$(USER_SOURCES)))
USER_PROGRAM_FILES     := $(patsubst %/,%.bin,$(sort $(dir $(USER_OBJECT_FILES))))

get_user_bin_deps_objects = $(filter $(patsubst %.bin,%/,$(1))%,$(USER_OBJECT_FILES))
