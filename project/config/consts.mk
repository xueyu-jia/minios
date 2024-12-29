#! minios pragma: export
#! supported boot impls under arch/<arch>/boot
SUPPORT_BOOT_TYPES := floppy mbr

#! minios pragma: export
#! where to find presets for minios build
MINIOS_PRESET_FILE := minios.preset

#! minios pragma: export
#! default size of the image files
DEFAULT_IMAGE_SIZE := 100m

#! minios pragma: export
#! default partition geometry file for the image files, used by sfdisk
DEFAULT_IMAGE_PART_GEO := part.sfdisk
