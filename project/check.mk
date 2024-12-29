$(call import,lib.core lib.io lib.string)
$(call import,config.utils)

$(call ensures_not_null,$(ARCH),no arch specified)
$(call assert,$(call is_dir,arch/$(ARCH)),arch $(ARCH) is not supported)

$(call ensures_not_null,$(ARCH_BITWIDTH),no bitwidth specified)
$(call assert,$(call contains,32 64,$(ARCH_BITWIDTH)),invalid arch bitwidth $(ARCH_BITWIDTH))

$(call ensures_not_null,$(ARCH_ELF_TYPE),no arch elf type specified)

ARCH_AVAILABLE_BOOT_TYPES := $(call arch_get_boot_types,$(ARCH))
$(call ensures_not_null,$(ARCH_AVAILABLE_BOOT_TYPES),no available boot found for arch $(ARCH))

$(call assert,$(call is_machine_hex,$(KERNEL_START_ADDR)),invalid kernel start address: $(call or_else,$(KERNEL_START_ADDR),<null>))

SPECIFIED_BOOT_TYPE_AVAILABLE := $(if $(BOOT),$(call contains,$(ARCH_AVAILABLE_BOOT_TYPES),$(BOOT)),1)
$(call assert,$(SPECIFIED_BOOT_TYPE_AVAILABLE),boot type $(BOOT) is not supported for arch $(ARCH))

$(call ensures_not_null,$(BOOTFS),fs for boot partition is not specified)
ifeq ($(BOOTFS_PART_INDEX),)
$(call assert,$(call is_null,$(filter-out floppy,$(if $(BOOT),$(BOOT),$(ARCH_AVAILABLE_BOOT_TYPES)))), \
	expect boot partition index for non floppy boot type)
endif

$(call ensures_not_null,$(ROOTFS),no rootfs specified)
ifeq ($(ROOTFS_PART_INDEX),)
$(call assert,$(call is_null,$(filter-out floppy,$(if $(BOOT),$(BOOT),$(ARCH_AVAILABLE_BOOT_TYPES)))), \
	expect rootfs partition index for non floppy boot type)
endif

ifeq ($(BOOTFS_PART_INDEX),$(ROOTFS_PART_INDEX))
$(call assert,$(filter $(BOOTFS),$(ROOTFS)),different fs specified on partition $(BOOTFS_PART_INDEX))
endif

ifneq ($(INSDEV),)
$(call assert,$(call exists,$(INSDEV)),specified install device not exists)
$(call assert,$(call scmp_eq,$(shell stat -c %F '$(INSDEV)' $(ERR2NUL)),block special file),specified install device is not a block device)
$(call assert,$(call regex_match,^/dev/sd[a-z]$$,$(INSDEV)),specified install device is not a scsi hard disk)
#! TODO: further check the type of the hard disk
endif

ifneq ($(BOOT),floppy)
$(call assert,$(call regex_match,^[1-9][0-9]*[km]?$$,$(call to_lower,$(IMAGE_SIZE))),invalid image size: $(call or_else,$(IMAGE_SIZE),<null>))
$(call assert,$(call exists,$(IMAGE_PART_GEO)),partition geometry file not exists)
endif
