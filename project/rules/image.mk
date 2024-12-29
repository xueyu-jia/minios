$(call import,lib.core)

#! TODO: floppy not supported yet

$(call filter_image_files,floppy,$(IMAGE_FILES)): $(call get_image_deps,floppy,boot loader) $(KERNEL_FILE)
	@mkdir -p $(dir $@)
	@$(call enter-job,setup,$(notdir $@))
	@dd if=/dev/zero of=$@ bs=1024 count=1440 $(ALL2LOG)
	@$(call leave-job,done,setup,$(notdir $@))

#! TODO: support grub partition

$(call filter_image_files,mbr,$(IMAGE_FILES)): $(call get_image_deps,mbr,mbr boot loader) $(KERNEL_FILE) $(IMAGE_PART_GEO)
	@mkdir -p $(dir $@)
	@$(call enter-job,setup,$(notdir $@))
	@dd if=/dev/zero of=$@ bs=$(IMAGE_SIZE_UNIT) count=$(IMAGE_SIZE_COUNT) $(ALL2LOG)
	@sfdisk $@ < $(lastword $^) $(ALL2LOG)
	@dd if=$(word 1,$^) of=$@ bs=1 count=446 conv=notrunc $(ALL2LOG)
	@if [ -true ]; then \
		loop_device=`losetup -f`;                                                                      \
		sudo losetup -P $$loop_device $@;                                                              \
		                                                                                               \
		bootfs_part=$${loop_device}p$(BOOTFS_PART_INDEX);                                              \
		sudo $(call fstool_get,make,$(BOOTFS)) $$bootfs_part;                                          \
		                                                                                               \
		rootfs_part=$${loop_device}p$(ROOTFS_PART_INDEX);                                              \
		sudo $(call fstool_get,make,$(ROOTFS)) $$rootfs_part;                                          \
		                                                                                               \
		boot_seek=$(call fstool_lookup,boot_seek,$(BOOTFS));                                           \
		boot_size=$$[ 512 - $$boot_seek ];                                                             \
		sudo dd if=$(word 2,$^) of=$$bootfs_part bs=1 count=$$boot_size seek=$$boot_seek conv=notrunc; \
		                                                                                               \
		mount_point=$$(mktemp -d);                                                                     \
		sudo $(call fstool_get,mount,$(BOOTFS)) $$bootfs_part $$mount_point;                           \
		sudo cp $(word 3,$^) $$mount_point/loader.bin;                                                 \
		sudo cp $(word 4,$^) $$mount_point/kernel.bin;                                                 \
		sudo umount $$mount_point;                                                                     \
		                                                                                               \
		sudo losetup -d $$loop_device;                                                                 \
	fi $(ALL2LOG)
	@$(call leave-job,done,setup,$(notdir $@))
