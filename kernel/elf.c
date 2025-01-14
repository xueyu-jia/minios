#include <minios/elf.h>
#include <minios/vfs.h>

void elf_read_header(int fd, elf32_hdr_t *eh, ptrdiff_t offset) {
    kern_vfs_lseek(fd, offset, SEEK_SET);
    kern_vfs_read(fd, (void *)eh, sizeof(elf32_hdr_t));
}

void elf_read_phdr(int fd, elf32_phdr_t *ph, ptrdiff_t offset) {
    kern_vfs_lseek(fd, offset, SEEK_SET);
    kern_vfs_read(fd, (void *)ph, sizeof(elf32_phdr_t));
}

void elf_read_shdr(int fd, elf32_shdr_t *sh, ptrdiff_t offset) {
    kern_vfs_lseek(fd, offset, SEEK_SET);
    kern_vfs_read(fd, (void *)sh, sizeof(elf32_shdr_t));
}

int read_elf(int fd, elf32_hdr_t *eh, elf32_phdr_t *ph, elf32_shdr_t *sh) {
    elf_read_header(fd, eh, 0);
    if (!(eh->e_magic == ELF_MAGIC && eh->e_machine == EM_386)) { return -1; }
    for (int i = 0; i < eh->e_phnum; ++i) {
        elf_read_phdr(fd, &ph[i], eh->e_phoff + eh->e_phentsize * i);
    }
    for (int i = 0; i < eh->e_shnum; ++i) {
        elf_read_shdr(fd, &sh[i], eh->e_shoff + eh->e_shentsize * i);
    }
    return 0;
}
