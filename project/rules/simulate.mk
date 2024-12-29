$(call import,lib.core lib.env)

$(call export_icase_var,DEBUG)
$(call export_icase_var,TARGET)
$(call export_icase_var,REALMODE)

simulate: force
monitor: force

ifneq ($(call contains,$(IMAGE_BOOT_TYPES),$(TARGET)),)
simulate: $(call filter_image_files,$(TARGET),$(IMAGE_FILES))
	@$(QEMU) $(call get_qemu_flags,$<,) $(if $(DEBUG),-s -S,)
else
simulate:
	$(error invalid simulation target: $(call or_else,$(TARGET),<null>))
endif

monitor: $(KERNEL_DEBUG_FILE) $(if $(REALMODE),$(GDB_REALMODE_XML) $(GDB_REALMODE_SCRIPT),)
ifeq ($(REALMODE),)
	@$(GDB) $(GDB_FLAGS) \
		-ex 'layout-sac' \
		-ex 'file $<'    \
		-ex 'b _start'   \
		-ex 'c'
else
	@$(GDB) $(GDB_FLAGS)                      \
		-ex 'set tdesk filename $(word 2,$^)' \
		-x '$(word 3,$^)'                     \
		-ex 'file $<'                         \
		-ex 'enter-real-mode'                 \
		-ex 'b *0x7c00'                       \
		-ex 'c'
endif
