$(call import,lib.core)

QEMU ?= qemu-system-$(ARCH)

QEMU_DISPLAY ?=
QEMU_MEMORY  ?= 64

QEMU_FLAG_AHCI_DEVICE = -device ich9-ahci,id=ahci
QEMU_FLAG_EHCI_DEVICE = -device usb-ehci,id=ehci

QEMU_FLAGS_TEMPLATE ?=
QEMU_FLAGS_TEMPLATE += -m $(QEMU_MEMORY)m,slots=4,maxmem=4g
QEMU_FLAGS_TEMPLATE += $(foreach i,$(call range,4),                                       \
-object memory-backend-file,size=32m,id=ram$(i),mem-path=ram$(i).mem,share=on,prealloc=on \
-device pc-dimm,id=dimm$(i),memdev=ram$(i)                                                \
)
QEMU_FLAGS_TEMPLATE += $(if $(1),$(QEMU_FLAG_AHCI_DEVICE),)
QEMU_FLAGS_TEMPLATE += $(foreach i,$(call range,$(words $(1))),               \
-drive if=none,id=hdd$(i),file=$(word $(call arith_inc,$(i)),$(1)),format=raw \
-device ide-hd,bus=ahci.$(i),drive=hdd$(i)                                    \
)
QEMU_FLAGS_TEMPLATE += $(if $(2),$(QEMU_FLAG_EHCI_DEVICE),)
QEMU_FLAGS_TEMPLATE += $(foreach i,$(call range,$(words $(2))),               \
-drive if=none,id=fhd$(i),file=$(word $(call arith_inc,$(i)),$(2)),format=raw \
-device usb-storage,bus=ehci.$(i),drive=fhd$(i)                               \
)
QEMU_FLAGS_TEMPLATE += -rtc base=utc
QEMU_FLAGS_TEMPLATE += -boot menu=on,splash-time=3000
QEMU_FLAGS_TEMPLATE += -device isa-serial,chardev=com1
QEMU_FLAGS_TEMPLATE += -chardev null,id=com1,logfile=$(OBJDIR)minios-$(ARCH).serial.$(shell date +%Y%m%d%H).log,logappend=on
QEMU_FLAGS_TEMPLATE += $(foreach i,$(call range,2,5),              \
-device isa-serial,chardev=com$(i)                                 \
-chardev null,id=com$(i),logfile=com$(i)-redirect.log,logappend=on \
)
QEMU_FLAGS_TEMPLATE += -device isa-parallel,chardev=lpt1
QEMU_FLAGS_TEMPLATE += -chardev null,id=lpt1,logfile=lpt1-redirect.log,logappend=on
QEMU_FLAGS_TEMPLATE += $(if $(QEMU_DISPLAY),-display $(QEMU_DISPLAY),)

#! get_qemu_flags <hdd-list> <fhd-list>
get_qemu_flags = $(call format_list,$(call QEMU_FLAGS_TEMPLATE,$(1),$(2)))
