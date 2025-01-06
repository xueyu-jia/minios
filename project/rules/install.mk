$(call import,lib.core)

AVAILABLE_INSTALL_TARGETS := $(foreach type,$(IMAGE_BOOT_TYPES),install-$(type))

define declare_install_deps
install-$(1): $(call filter_image_files,$(1),$(IMAGE_FILES)) $(ARCH_FLAGS_DIR)$(1) $(USER_PROGRAM_FILES) force
install: install-$(1)
endef
$(foreach type,$(IMAGE_BOOT_TYPES),$(eval $(call declare_install_deps,$(type))))

get_rootfs_part_suffix = $(if $(filter-out floppy,$(1)),p$(ROOTFS_PART_INDEX),)
get_rootfs_part_suffix_from_flag = $(call get_rootfs_part_suffix,$(patsubst $(ARCH_FLAGS_DIR)%,%,$(1)))

#! TODO: support config scripts for custom mount options
get_user_install_dest = $(call format_list,                              \
	$(if $(filter $(USER_OBJDIR)$(USER_TESTSUIT_ID)/%,$(1)),             \
		$(patsubst $(USER_OBJDIR)$(USER_TESTSUIT_ID)/%.bin,test/%,$(1)), \
		$(patsubst $(USER_OBJDIR)%.bin,%,$(1))))

#! install_target_file <prefix> <file> <dest>
define install_target_file
	sudo mkdir -p $(dir $(1)$(3)); \
	sudo cp $(2) $(1)$(3)
endef

$(AVAILABLE_INSTALL_TARGETS):
	@$(call enter-job,install $(words $(USER_PROGRAM_FILES)) user programs,)
	@if [ -true ]; then \
		loop_device=`losetup -f`;                                                                      \
		sudo losetup -P $$loop_device $<;                                                              \
		rootfs_part=$${loop_device}$(call get_rootfs_part_suffix_from_flag,$(word 2,$^));              \
		mount_point=$$(mktemp -d);                                                                     \
		sudo $(call fstool_get,mount,$(ROOTFS)) $$rootfs_part $$mount_point;                           \
		$(foreach bin,$(USER_PROGRAM_FILES),                                                           \
			$(call install_target_file,$${mount_point}/,$(bin),$(call get_user_install_dest,$(bin)));) \
		sudo umount $$mount_point;                                                                     \
		sudo losetup -d $$loop_device;                                                                 \
	fi $(ALL2LOG)
	@$(call leave-job,done,install $(words $(USER_PROGRAM_FILES)) user programs,)
