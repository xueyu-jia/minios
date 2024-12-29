$(call import,lib.string)

#! minios pragma: export
VALID_ENV_NAME_PATTERN := ^[A-Z_][A-Z0-9_]*$$

list-envs: force #<! list available envs for configuration
	@$(foreach v,$(.VARIABLES),$(foreach e,$(call regex_search,$(VALID_ENV_NAME_PATTERN),$(v)),$(info $(e) = $($(e)))))

$(ENVS_FILE): force
	@mkdir -p $(@D)
	@$(NESTED_MAKE) -s list-envs $(ERR2NUL) | grep -P '^\w+\s*=.*$$' > $@.swp
	@cmp -s $@ $@.swp || cp $@.swp $@
	@rm -f $@.swp

define configure-file
	vars=`cat $(1) | grep -Po '(?<=@)\w+(?=@)' | tr '\n' ' '`;     \
	cat $(ENVS_FILE) | while IFS= read -r row; do                  \
		read -r key value <<<                                      \
			`echo "$${row}" | awk -F' = ' '{print $$1, $$2}'`;     \
		if [[ " $${vars} " =~ " $${key} " ]]; then                 \
			subst=$$(echo "$${value}" | sed -r 's/([/\])/\\\1/g'); \
			sed -i "s/@$${key}@/$${subst}/g" $(1);                 \
		fi;                                                        \
	done
endef

define declare_config_gen_deps
$(call config_in2gen,$(1)): $(1) $(ENVS_FILE)
endef
$(foreach config_in,$(CONFIG_IN_FILES),$(eval $(call declare_config_gen_deps,$(config_in))))

$(call config_in2gen,$(CONFIG_IN_FILES)):
	@mkdir -p $(@D)
	@$(call enter-job,config,$<)
	@cp $< $@.swp
	@$(call configure-file,$@.swp);       \
	if cmp -s $@ $@.swp; then             \
		$(call leave-job,skip,config,$<); \
	else                                  \
		cp $@.swp $@;                     \
		$(call leave-job,done,config,$<); \
	fi
	@rm -f $@.swp
