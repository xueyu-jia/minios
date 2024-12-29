$(call import,lib.core lib.string)

exists  = $(call test,-e '$(1)')
is_dir  = $(call test,-d '$(1)')
is_file = $(call test,-f '$(1)')

_find_ext_list_not_null = $(if $(1),$(1),$(error find_files_recursive: no extension specified))
_find_concat_ext = $(patsubst <TAG>%,-name '*.%',$(call join_parts,$(addprefix <TAG>,$(1)), -o ))
_find_posix_regex = -regextype posix-extended -regex '$(1)'
_find_files_recursive = $(shell [ -d '$(1)' ] && find '$(1)' $(if $(3),$(call _find_posix_regex,$(3)) -prune -o,) $(call _find_concat_ext,$(2)))

#! find_files_recursive <search-dir> <ext-list> <exclude-regex>
find_files_recursive = $(patsubst ./%,%,$(call _find_files_recursive,$(if $(1),$(1),.),$(call _find_ext_list_not_null,$(2)),$(3)))
