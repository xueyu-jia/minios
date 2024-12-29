$(call import,config.consts)

#! minios pragma: required
#! target architecture
#! use `list-arch` to see available architectures
ARCH ?=

#! minios pragma: required
#! bit width of the target architecture, only 32 or 64 are supported
ARCH_BITWIDTH ?=

#! minios pragma: required
#! elf type of the output binaries for the target architecture
ARCH_ELF_TYPE ?=

#! minios pragma: required
#! kernel elf start address, in hexadecimal format
KERNEL_START_ADDR ?=

#! minios pragma: optional
#! boot type of the image, if none, build all supported types
#! use `list-boot` to see available boot types
BOOT ?=

#! minios pragma: required
#! fs type of the boot partition in the image
BOOTFS ?=

#! minios pragma: optional
#! boot partition index in the image, can be null only if BOOT is floppy
BOOTFS_PART_INDEX ?=

#! minios pragma: required
#! rootfs type of the image
ROOTFS ?=

#! minios pragma: optional
#! rootfs partition index in the image, can be null only if BOOT is floppy
ROOTFS_PART_INDEX ?=

#! minios pragma: optional
#! the device to install the image to, install to image file if not specified
INSDEV ?=

#! minios pragma: optional
#! the size of the image file to create and install to, not used for floppy image
IMAGE_SIZE ?= $(DEFAULT_IMAGE_SIZE)

#! minios pragma: optional
#! partition geometry file for the image file, used by sfdisk, not used for floppy image
IMAGE_PART_GEO ?= $(DEFAULT_IMAGE_PART_GEO)
