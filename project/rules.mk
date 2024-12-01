$(call import,deprecated)
$(call import,core fmt)

$(COMPILE_DB_FILE): force
	@$(call enter-job,dump,$(notdir $@))
	@mkdir -p $(@D)
	@\
	if bear --output $@ -- $(MAKE) -B ENABLE_BUILD_LOG= $(OUT2NUL) $(ERR2LOG); then \
		$(call leave-job,done,dump,$(notdir $@));                                   \
	else                                                                            \
		$(call leave-job,fail,dump,$(notdir $@));                                   \
	fi
.PHONY: force

help: force #<! display this information
	@awk 'BEGIN {FS = ":.*?#<! "} /^[a-zA-Z_-]+:.*?#<! / {sub("\\\\n",sprintf("\n%22c"," "), $$2);printf " \033[36m%-12s\033[0m  %s\n", $$1, $$2}' $(MAKEFILE_LIST)
.PHONY: force

format: force #<! format *.c and *.h files using clang-format
	@$(call enter-job,format sources)
ifeq ($(CLANG_FORMAT_EXECUTABLE),)
	@$(call leave-job,fail,clang-format version $(CLANG_FORMAT_VERSION) is not available,)
else
	@$(CLANG_FORMAT_EXECUTABLE) -i `git ls-files *.c *.h`
	@$(call leave-job,done,format sources,)
endif
.PHONY: force

dup-cc-win: $(COMPILE_DB_FILE) #<! dump clangd compile_commands.json for windows
	@sed -i -re 's/(\/[a-z]+)*\/?(gcc|g\+\+)/\2/g' $<
	@sed -i -r 's/\/mnt\/([a-z])\//\1:\//g' $<
	@sed -i -re 's/(\r?\n?)+[\t ]*"\-f(\w\-?)+",//g' -e '/^$$/d' $<

dup-cc: $(COMPILE_DB_FILE) #<! dump clangd compile_commands.json
config: dup-cc $(GENERATED_FILES) #<! configure project
config-win: dup-cc-win $(GENERATED_FILES) #<! configure project for windows

conf: config #<! alias for `config`
conf-win: config-win #<! alias for `config-win`
fmt: format #<! alias for `format`
