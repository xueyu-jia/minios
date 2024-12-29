#! minios pragma: optional
CLANG_FORMAT_VERSION ?= 18

#! minios pragma: export
CLANG_FORMAT_EXECUTABLE := $(shell \
prefix=clang-format;                                                            \
executables=`$(call listcmds_defer,$$prefix-$(CLANG_FORMAT_VERSION) $$prefix)`; \
pat='(?<=version )(\d+)(?=\.\d+\.\d+)';                                         \
for formatter in $$executables; do                                              \
    major_version=`$$formatter --version | grep -Po "$$pat"`;                   \
    [ "$$major_version" -eq "$(CLANG_FORMAT_VERSION)" ] && break;               \
    formatter=;                                                                 \
done;                                                                           \
echo $$formatter;                                                               \
)
