#pragma once

#include <klib/stdint.h>
#include <klib/stddef.h>
#include <sys/elf.h>

int read_elf(int fd, elf32_hdr_t *eh, elf32_phdr_t *ph, elf32_shdr_t *sh);
void elf_read_header(int fd, elf32_hdr_t *eh, ptrdiff_t offset);
void elf_read_phdr(int fd, elf32_phdr_t *ph, ptrdiff_t offset);
void elf_read_shdr(int fd, elf32_shdr_t *sh, ptrdiff_t offset);
