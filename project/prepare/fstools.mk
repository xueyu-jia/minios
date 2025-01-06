$(call import,lib.core lib.string)

#! fstool_unwrap_local_deps <config>
fstool_unwrap_local_deps = $(call format_list,$(if $(1),$(foreach n,$(words $(1)),                                  \
	$(if $(call and,$(filter 2,$(n)),$(filter local,$(word 1,$(1)))),$(patsubst tools/%,$(OBJDIR)%,$(word 2,$(1))), \
	$(if $(filter 1,$(n)),,$(error invalid fs tool config: $(1))))),))

#! fstool_get <tool-name> <fs>
_fstool_get_tool = $(foreach t,$(2)_FS_$(call to_upper,$(1)), \
	$(call or_else,$(call fstool_unwrap_local_deps,$(call safe_deref,$(t))),$(call safe_deref,$(t))))
_fstool_get_tool_strict = $(call or_else,$(call _fstool_get_tool,$(1),$(2)),)
_fstool_get = $(foreach tool,$(call _fstool_get_tool_strict,$(1),$(2)),    \
	$(if $(tool),$(tool),$(error fs tool $(2) for $(1) is not available))) \
	$(call safe_deref,$(2)_FS_$(call to_upper,$(1))_FLAGS)
fstool_get = $(call format_list,$(call _fstool_get,$(1),$(2)))

#! fstool_lookup <config-name> <fs>
fstool_lookup = $(call format_list,$(foreach t,$(2)_FS_$(call to_upper,$(1)), \
	$(if $(filter-out undefined,$(origin $(t))),                              \
		$(call safe_deref,$(t)),                                              \
		$(error fs config $(call or_else,$(1),???) for $(call or_else,$(2),???) not found))))

#! mkfs -> $(call fstool_get,make,<fs>) <image>
#! mount -> $(call fstool_get,mount,<fs>) <image> <mount-point>

fat32_FS_MAKE        := mkfs.vfat
fat32_FS_MAKE_FLAGS  := -F 32 -s 8
fat32_FS_MOUNT       := mount
fat32_FS_MOUNT_FLAGS :=
fat32_FS_BOOT_SEEK   := 90

orangefs_FS_MAKE        := local tools/mkfs.orangefs
orangefs_FS_MAKE_FLAGS  :=
orangefs_FS_MOUNT       := local tools/mount.orangefs
orangefs_FS_MOUNT_FLAGS :=
orangefs_FS_BOOT_SEEK   := 0
