$(call import,core string)

tm_raw = $(shell date '+%Y %m %d %H %M %S %N %:z')

tm_get          = $(call tm_get_$(1),$(2))
tm_get_year     = $(call index,0,$1)
tm_get_mon      = $(call index,1,$1)
tm_get_day      = $(call index,2,$1)
tm_get_hour     = $(call index,3,$1)
tm_get_min      = $(call index,4,$1)
tm_get_sec      = $(call index,5,$1)
tm_get_ns       = $(call index,6,$1)
tm_get_timezone = $(call index,7,$1)
tm_get_ms       = $(shell $(ECHO) $(call tm_get_ns,$(1)) / 1000000 | bc)
tm_get_date     = $(call join_parts,$(call slice,$(1),0,3),-)
tm_get_time     = $(call join_parts,$(call slice,$(1),3,3),:)

_datetime_ms   = $(shell printf "%s %s.%03d" "$(call tm_get_date,$(1))" "$(call tm_get_time,$(1))" "$(call tm_get_ms,$(1))")
_datetime_full = $(shell printf "%s %s" "$(call _datetime_ms,$(1))" "$(call tm_get_timezone,$(1))")

datetime      = $(shell date '+%Y-%m-%d %H:%M:%S')
datetime_ms   = $(call _datetime_ms,$(call tm_raw))
datetime_full = $(call _datetime_full,$(call tm_raw))
