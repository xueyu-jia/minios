$(call import,time string)

#! minios pragma: required
BUILD_LOG_FILE ?=
$(call ensures_not_null,$(BUILD_LOG_FILE),BUILD_LOG_FILE not defined)

#! minios pragma: optional
ENABLE_BUILD_LOG ?= 1

BUILD_LOG_ENV_DATETIME := $(call datetime_full)
BUILD_LOG_ENV_GOALS    := $(call select,-z "$(MAKECMDGOALS)",<default>,$(MAKECMDGOALS))

ifneq ($(ENABLE_BUILD_LOG),)
define setup-build-log
	mkdir -p $(dir $(BUILD_LOG_FILE));                              \
	$(ECHO) "---" >> $(BUILD_LOG_FILE);                             \
	$(ECHO) "Date: $(BUILD_LOG_ENV_DATETIME)" >> $(BUILD_LOG_FILE); \
	$(ECHO) "Goal: $(BUILD_LOG_ENV_GOALS)" >> $(BUILD_LOG_FILE);    \
	$(ECHO) "..." >> $(BUILD_LOG_FILE);
endef
$(shell $(call setup-build-log))
endif

define _log
	msg="$(1)";                                                           \
	$(ECHO) -ne "$$msg";                                                  \
	csi_pat='s/\\e\[((\?[0-9]{2,4}[hl])|([0-9]{1,2}(;[0-9]{1,2})*m))//g'; \
	msg=$$($(ECHO) "$$msg" | sed -r $$csi_pat);                           \
	msg=$$($(ECHO) "$$msg" | sed -r 's/\\r/\\n/g');                       \
	$(ECHO) -ne "$$msg" >> $(BUILD_LOG_FILE)
endef
log = $(if $(ENABLE_BUILD_LOG),$(call _log,$(1)),)

OUT2LOG = $(if $(ENABLE_BUILD_LOG),>> $(BUILD_LOG_FILE),)
ERR2LOG = $(if $(ENABLE_BUILD_LOG),2>> $(BUILD_LOG_FILE),)
ALL2LOG = $(if $(ENABLE_BUILD_LOG),>> $(BUILD_LOG_FILE) 2>&1,)
OUT2NUL = > /dev/null
ERR2NUL = 2> /dev/null
ALL2NUL = > /dev/null 2>&1

job-label-proc = \e[0m[PROC]\e[0m
job-label-done = \e[32m[DONE]\e[0m
job-label-skip = \e[33m[SKIP]\e[0m
job-label-fail = \e[31m[FAIL]\e[0m

job-label = $(job-label-$(1))

enter-job = $(call log,\e[?25l$(call datetime_ms) $(call job-label,proc) $(call join_parts,$(1) $(2), )\r)
leave-job = $(call log,$(call datetime_ms) $(call job-label,$(1)) $(call join_parts,$(2) $(3), )\e[?25h\n)
