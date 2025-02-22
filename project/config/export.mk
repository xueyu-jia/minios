$(call import,config.consts)

#! minios pragma: begin_export

#! where is the sources of the current arch
ARCH_DIR := arch/$(ARCH)/

#! headers maybe used by kernel scope modules
KERNEL_SCOPE_ENTRIES := minios klib uapi driver fs

#! headers maybe used by user scope modules
USER_SCOPE_ENTRIES := ulib

#! core modules to be included into the kernel compilation
CORE_MINIOS_MODULES := kernel mm ipc

#! all modules to be included into the kernel compilation
MINIOS_MODULES := arch/$(ARCH) $(CORE_MINIOS_MODULES) $(KERNEL_SCOPE_ENTRIES)

#! all modules to be included into the user compilation
USER_MODULES := user $(ARCH_DIR)user test $(USER_SCOPE_ENTRIES)

#! where to place the output of the user testcase programs
USER_TESTSUIT_ID := TEST{B9E6C8C7-8159-47EE-855B-2BEA755FB3F3}

#! a generic library provided to both minios and user programs
LIBGENERIC_DIR           := generic/
LIBGENERIC_SOURCE_DIR    := $(LIBGENERIC_DIR)src/
LIBGENERIC_INTERFACE_DIR := $(LIBGENERIC_DIR)include/

#! where to place the output of the target category
ARCH_OBJDIR    := $(OBJDIR)arch-$(ARCH)/
HOST_OBJDIR    := $(OBJDIR)
TOOL_OBJDIR    := $(HOST_OBJDIR)tools/
GENERIC_OBJDIR := $(ARCH_OBJDIR)$(LIBGENERIC_DIR)
BOOT_OBJDIR    := $(ARCH_OBJDIR)arch/boot/
KERNEL_OBJDIR  := $(ARCH_OBJDIR)
ULIB_OBJDIR    := $(ARCH_OBJDIR)ulib/
USER_OBJDIR    := $(ARCH_OBJDIR)user/
CONFIG_OBJDIR  := $(ARCH_OBJDIR).generated/

#! output libraries
LIBGENERIC_FILE := $(ARCH_OBJDIR)libc.a
LIBULIB_FILE    := $(ARCH_OBJDIR)libu.a
LIBKLIB_FILE    := $(ARCH_OBJDIR)libk.a

#! output binaries
KERNEL_FILE       := $(ARCH_OBJDIR)kernel.bin
KERNEL_DEBUG_FILE := $(ARCH_OBJDIR)kernel.dbg

#! minios pragma: end_export
