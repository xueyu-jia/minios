HOST_AS := gcc
HOST_CC := gcc
HOST_LD := ld
HOST_AR := ar

HOST_AS_FLAGS :=
HOST_CC_FLAGS :=
HOST_CC_FLAGS += -w
HOST_CC_FLAGS += -O3
HOST_AR_FLAGS := rcs
HOST_LD_FLAGS :=

ARCH_AS := nasm
ARCH_CC := gcc
ARCH_AR := ar
ARCH_LD := ld

ARCH_AS_FLAGS :=
ARCH_CC_FLAGS :=
ARCH_CC_FLAGS += -std=gnu99
ARCH_CC_FLAGS += -g -O0
ARCH_CC_FLAGS += -static -fno-pie
ARCH_CC_FLAGS += -Wall -Wextra -Werror
ARCH_CC_FLAGS += -nostdinc -nostdlib
ARCH_CC_FLAGS += -fno-stack-protector -fno-builtin
ARCH_CC_FLAGS += -mno-sse -mno-sse2 -mno-mmx
ARCH_CC_FLAGS += -m$(ARCH_BITWIDTH)
ARCH_AR_FLAGS :=
ARCH_AR_FLAGS += rcs
ARCH_LD_FLAGS :=
ARCH_LD_FLAGS += -m elf_$(ARCH_ELF_TYPE)

GENERIC_INCDIRS := $(LIBGENERIC_INTERFACE_DIR)
BOOT_INCDIRS    :=
BOOT_INCDIRS    += $(LIBGENERIC_INTERFACE_DIR)
BOOT_INCDIRS    += $(ARCH_DIR)boot/include
KERNEL_INCDIRS  :=
KERNEL_INCDIRS  += $(LIBGENERIC_INTERFACE_DIR)
KERNEL_INCDIRS  += $(ARCH_DIR)include
KERNEL_INCDIRS  += include
ULIB_INCDIRS    :=
ULIB_INCDIRS    += include/ulib
ULIB_INCDIRS    += $(LIBGENERIC_INTERFACE_DIR)
ULIB_INCDIRS    += include
USER_INCDIRS    :=
USER_INCDIRS    += include/ulib
USER_INCDIRS    += $(LIBGENERIC_INTERFACE_DIR)

GENERIC_CFLAGS :=
BOOT_CFLAGS    :=
KERNEL_CFLAGS  :=
ULIB_CFLAGS    :=
USER_CFLAGS    :=
USER_CFLAGS    += -w

$(call import,lib.core)
expand_incdirs = $(if $(1),$(call format_list,$(foreach dir,$(call safe_deref,$(1)_INCDIRS),$(dir) $(CONFIG_OBJDIR)$(dir)),))

#! get_*_incdir_flags <category> <source-path>
get_as_incdir_flags = $(addprefix -I,$(dir $(2)) $(CONFIG_OBJDIR)$(dir $(2)) $(call expand_incdirs,$(1)))
get_cc_incdir_flags = $(addprefix -iquote ,$(dir $(2)) $(CONFIG_OBJDIR)$(dir $(2))) \
	$(addprefix -I,$(call expand_incdirs,$(1))) $(call safe_deref,$(1)_CFLAGS)

#! get_arch_* <category>
get_arch_as = $(ARCH_AS) $(ARCH_AS_FLAGS) $(call get_as_incdir_flags,$(1),$<)
get_arch_cc = $(ARCH_CC) $(ARCH_CC_FLAGS) $(call get_cc_incdir_flags,$(1),$<)
