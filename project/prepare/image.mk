$(call import,lib.core lib.string config.utils)

get_floppy_image_suffix = $(BOOTFS)+$(call or_else,$(ROOTFS),$(BOOTFS))
get_mbr_image_suffix = $(BOOTFS)+p$(ROOTFS_PART_INDEX)$(ROOTFS)

get_image_name = minios-$(ARCH)-$(1).$(call get_$(1)_image_suffix).img
get_image_deps = $(foreach target,$(2),$(ARCH_OBJDIR)$(1)_$(target).bin)
filter_image_files = $(foreach file,$(2),$(if $(filter $(call get_image_name,$(1)),$(notdir $(file))),$(file),))

IMAGE_BOOT_TYPES := $(call or_else,$(BOOT),$(call arch_get_boot_types,$(ARCH)))
IMAGE_FILES      := $(addprefix $(OBJDIR),$(foreach type,$(IMAGE_BOOT_TYPES),$(call get_image_name,$(type))))

IMAGE_SIZE_UNIT  := $(foreach size,$(call to_lower,$(IMAGE_SIZE)),$(if $(filter %m,$(size)),1048576,$(if $(filter %k,$(size)),1024,1)))
IMAGE_SIZE_COUNT := $(call regex_search,^[1-9][0-9]*,$(IMAGE_SIZE))
