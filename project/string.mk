COMMA = ,

quoted = "$(1)"

slice = $(shell raw="$(1)"; parts=($$raw); $(ECHO) $${parts[@]:$(2):$(3)})

join_parts = $(shell raw="$(1)"; $(ECHO) $${raw// /$(2)})

filter_csi = $(shell $(ECHO) "$1" | sed -r 's/\\e\[((\?[0-9]+[hl])|([0-9]?(;[0-9]+)*m))//g')
