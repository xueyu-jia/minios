$(call import,lib.string)

generic_o2s = $(patsubst $(GENERIC_OBJDIR)%.obj,$(LIBGENERIC_SOURCE_DIR)%,$(1))
generic_s2o = $(patsubst $(LIBGENERIC_SOURCE_DIR)%,$(GENERIC_OBJDIR)%.obj,$(1))

kernel_o2s = $(patsubst arch/%,$(ARCH_DIR)%,$(patsubst $(KERNEL_OBJDIR)%.obj,%,$(1)))
kernel_s2o = $(patsubst %,$(KERNEL_OBJDIR)%.obj,$(patsubst $(ARCH_DIR)%,arch/%,$(1)))

user_o2s = $(patsubst arch/%,$(ARCH_DIR)%,$(patsubst user/$(USER_TESTSUIT_ID)/%,test/%,$(patsubst $(ARCH_OBJDIR)%.obj,%,$(1))))
user_s2o = $(patsubst %,$(ARCH_OBJDIR)%.obj,$(patsubst test/%,user/$(USER_TESTSUIT_ID)/%,$(patsubst $(ARCH_DIR)%,arch/%,$(1))))

boot_get_bin_file = $(ARCH_OBJDIR)$(1)_$(2).bin
boot_extract_type_from_source = $(foreach src,$(1),$(call regex_search,^\w+(?=\/),$(patsubst $(ARCH_DIR)boot/%,%,$(src))))
boot_s2b = $(foreach src,$(1),$(call boot_get_bin_file,$(call boot_extract_type_from_source,$(src)),$(patsubst %.asm,%,$(notdir $(src)))))
