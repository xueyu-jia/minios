$(CACHE_DIR)%:
	@mkdir -p $(dir $@)
	@[ ! -e $@ ] && touch $@ || true
