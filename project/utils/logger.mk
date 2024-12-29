$(call import,lib.core lib.time lib.string)

#! minios pragma: required
BUILD_LOG_FILE ?=
$(call ensures_not_null,$(BUILD_LOG_FILE),BUILD_LOG_FILE not defined)

#! minios pragma: optional
ENABLE_BUILD_LOG ?= 1

#! minios pragma: optional
ENABLE_PRETTY_LOG ?= 1

BUILD_LOG_ENV_DATETIME := $(call datetime_full)
BUILD_LOG_ENV_GOALS    := $(__GOALS__)

OUT2LOG = $(if $(ENABLE_BUILD_LOG),>> $(BUILD_LOG_FILE),)
ERR2LOG = $(if $(ENABLE_BUILD_LOG),2>> $(BUILD_LOG_FILE),)
ALL2LOG = $(if $(ENABLE_BUILD_LOG),>> $(BUILD_LOG_FILE) 2>&1,)
OUT2NUL = > /dev/null
ERR2NUL = 2> /dev/null
ALL2NUL = > /dev/null 2>&1

ifneq ($(ENABLE_BUILD_LOG),)
define setup-build-log
	mkdir -p $(dir $(BUILD_LOG_FILE));                    \
	$(ECHO) "---" $(OUT2LOG);                             \
	$(ECHO) "Date: $(BUILD_LOG_ENV_DATETIME)" $(OUT2LOG); \
	$(ECHO) "Goal: $(BUILD_LOG_ENV_GOALS)" $(OUT2LOG);    \
	$(ECHO) "Args: $(__ARGS__)" $(OUT2LOG);               \
	$(ECHO) "..." $(OUT2LOG);
endef
$(shell $(call setup-build-log))
endif

ifneq ($(ENABLE_PRETTY_LOG),)
define _log
	msg="$(1)";                                                           \
	$(ECHO) -ne "$$msg";                                                  \
	csi_pat='s/\\e\[((\?[0-9]{2,4}[hl])|([0-9]{1,2}(;[0-9]{1,2})*m))//g'; \
	msg=$$($(ECHO) "$$msg" | sed -r $$csi_pat);                           \
	msg=$$($(ECHO) "$$msg" | sed -r 's/\\r/\\n/g');                       \
	$(ECHO) -ne "$$msg" $(OUT2LOG)
endef
else
define _log
	$(ECHO) -e "$(1)"; \
	$(ECHO) -e "$(1)" $(OUT2LOG)
endef
endif

log = $(if $(ENABLE_BUILD_LOG),$(call _log,$(1)),$(ECHO) $(OUT2NUL))

ifneq ($(ENABLE_PRETTY_LOG),)
job-label-proc = \e[0m[PROC]\e[0m
job-label-done = \e[32m[DONE]\e[0m
job-label-skip = \e[33m[SKIP]\e[0m
job-label-fail = \e[31m[FAIL]\e[0m

job-label = $(job-label-$(1))

enter-job = $(call log,\e[?25l$(call datetime_ms) $(call job-label,proc) $(call join_parts,$(1) $(2), )\r)
leave-job = $(call log,$(call datetime_ms) $(call job-label,$(1)) $(call join_parts,$(2) $(3), )\e[?25h\n)
else
enter-job = $(call log,+ $(call join_parts,$(1) $(2), ))
leave-job = $(ECHO) $(OUT2NUL)
endif
