#! minios pragma: optional
#! use a set of highlevel generic rules instead of the default
#! ATTENTION: this would cost much time to start the build process
PROJECT_USE_STRICT_RULES ?=

RULES_ORDERED_SEQ := cache $(if $(PROJECT_USE_STRICT_RULES),object,object-fast) \
	lib boot kernel user image install tools config simulate lsp
$(call import,$(addprefix rules.,$(RULES_ORDERED_SEQ)))

-include $(SOURCE_DEPS_FILE)

boot: #<! build bootloaders only
lib: $(LIBGENERIC_FILE) $(LIBULIB_FILE) $(LIBKLIB_FILE) #<! build libraries only
kernel: $(KERNEL_FILE) $(KERNEL_DEBUG_FILE) #<! build kernel only
user: #<! build user programs only, including testcases
image: $(IMAGE_FILES) #<! build image only, including bootloader and kernel
install: #<! install latest user files to image and target device
simulate: #<! simulate minios on qemu, no installation performed
monitor: #<! launch gdb with qemu remote debug mode

build: boot lib kernel user image #<! build all stuffs

all: build

help: force #<! display this information
	@awk 'BEGIN                                        \
	{                                                  \
		FS = ":.*?#<! "                                \
	}                                                  \
	/^[a-z-]+:.*?#<! /                                 \
	{                                                  \
		printf " \033[36m%-14s\033[0m  %s\n", $$1, $$2 \
	}' $(MAKEFILE_LIST)

clean: force #<! clean outputs files
ifneq ($(ARCH),)
	@\
	if [ ! -d $(ARCH_OBJDIR) ]; then exit; fi;                  \
	$(call enter-job,clean-up all stuffs,);                     \
	if rm -rf $(ARCH_OBJDIR) $(ARCH_CACHE_DIR) $(ERR2NUL); then \
		$(call leave-job,done,clean-up all stuffs,);            \
	else                                                        \
		$(call leave-job,fail,clean-up all stuffs,);            \
	fi
endif

config: $(CONFIG_GEN_FILES) #<! configure *.in files

$(call import,lib.core config.utils)
list-boot: force #<! list available boot types for current arch
	@printf "\e[36m[arch=$(ARCH)]\e[0m \e[32m%s\e[0m\n" '$(call or_else,$(call arch_get_boot_types,$(ARCH)),null)'

format: force #<! format *.c and *.h files using clang-format
	@$(call enter-job,format sources)
ifeq ($(CLANG_FORMAT_EXECUTABLE),)
	@$(call leave-job,fail,clang-format version $(CLANG_FORMAT_VERSION) is not available,)
else
	@$(CLANG_FORMAT_EXECUTABLE) -i `git ls-files *.c *.h`
	@$(call leave-job,done,format sources,)
endif

config-lsp: $(COMPILE_DB_FILE) #<! dump clangd compile_commands.json
config-lsp-win: $(COMPILE_DB_FILE) #<! dump clangd compile_commands.json for windows
	@sed -i -re 's/(\/[a-z]+)*\/?(gcc|g\+\+)/\2/g' $<
	@sed -i -r 's/\/mnt\/([a-z])\//\1:\//g' $<
	@sed -i -re 's/(\r?\n?)+[\t ]*"\-f(\w\-?)+",//g' -e '/^$$/d' $<

i: install #<! alias for `install`
sim: simulate #<! alias for `simulate`
run: simulate #<! alias for `simulate`
mon: monitor #<! alias for `monitor`
conf: config #<! alias for `config`
fmt: format #<! alias for `format`
conf-lsp: config-lsp #<! alias for `config-lsp`
confw-lsp: config-lsp #<! alias for `config-lsp-win`
