#! TODO: test module needs to be refactored, a test bin should be compiled from a set of test cases

USER_OBJECT_FILES      := $(filter $(USER_OBJDIR)%,$(call user_s2o,$(USER_SOURCES)))
ARCH_USER_OBJECT_FILES := $(filter $(ARCH_OBJDIR)arch/user/%.obj,$(call user_s2o,$(USER_SOURCES)))

USER_PROGRAM_FILES := \
	$(patsubst %/,%.bin,$(sort $(dir $(filter-out $(USER_OBJDIR)$(USER_TESTSUIT_ID)/%,$(USER_OBJECT_FILES))))) \
	$(patsubst %.c.obj,%.bin,$(filter $(USER_OBJDIR)$(USER_TESTSUIT_ID)/%,$(USER_OBJECT_FILES)))

get_user_bin_deps_objects = $(if $(filter-out $(USER_OBJDIR)$(USER_TESTSUIT_ID)/%,$(1)), \
	$(filter $(patsubst %.bin,%/,$(1))%,$(USER_OBJECT_FILES)),                           \
	$(patsubst %.bin,%.c.obj,$(1)))
