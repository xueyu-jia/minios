#pragma once

#include <stdint.h>

#define ELF_MAGIC 0x464c457f //<! i.e. '\x7f' 'E' 'L' 'F'

typedef struct elf32_hdr {
    uint32_t e_magic; //<! always ELF_MAGIC
    uint8_t e_elf[12];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf32_hdr_t;

typedef struct elf32_phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} elf32_phdr_t;

typedef struct elf32_shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} elf32_shdr_t;

//! values of elf32_phdr.p_type
enum {
    PT_NULL = 0,
    PT_LOAD = 1,
    PT_DYNAMIC = 2,
    PT_INTERP = 3,
    PT_NOTE = 4,
    PT_SHLIB = 5,
    PT_PHDR = 6,
    PT_TLS = 7,
    PT_LOOS = 0x60000000,
    PT_HIOS = 0x6fffffff,
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7fffffff,
};

//! flag bits of elf32_phdr.p_flags
enum {
    ELF_PROG_FLAG_EXEC = 1,
    ELF_PROG_FLAG_WRITE = 2,
    ELF_PROG_FLAG_READ = 4,
};

//! values of elf32_shdr.sh_type
enum {
    ELF_SHT_NULL = 0,
    ELF_SHT_PROGBITS = 1,
    ELF_SHT_SYMTAB = 2,
    ELF_SHT_STRTAB = 3,
};

//! values of elf32_shdr.sh_name
enum {
    ELF_SHN_UNDEF = 0,
};

//! values of elf32_hdr.e_machine
enum {
    EM_NONE = 0,
    EM_M32 = 1,
    EM_SPARC = 2,
    EM_386 = 3,
    EM_68K = 4,
    EM_88K = 5,
    EM_860 = 7,
    EM_MIPS = 8,
    EM_S370 = 9,
    EM_MIPS_RS3_LE = 10,
    EM_PARISC = 15,
    EM_VPP500 = 17,
    EM_SPARC32PLUS = 18,
    EM_960 = 19,
    EM_PPC = 20,
    EM_PPC64 = 21,
    EM_S390 = 22,
    EM_V800 = 36,
    EM_FR20 = 37,
    EM_RH32 = 38,
    EM_RCE = 39,
    EM_ARM = 40,
    EM_ALPHA = 41,
    EM_SH = 42,
    EM_SPARCV9 = 43,
    EM_TRICORE = 44,
    EM_ARC = 45,
    EM_H8_300 = 46,
    EM_H8_300H = 47,
    EM_H8S = 48,
    EM_H8_500 = 49,
    EM_IA_64 = 50,
    EM_MIPS_X = 51,
    EM_COLDFIRE = 52,
    EM_68HC12 = 53,
    EM_MMA = 54,
    EM_PCP = 55,
    EM_NCPU = 56,
    EM_NDR1 = 57,
    EM_STARCORE = 58,
    EM_ME16 = 59,
    EM_ST100 = 60,
    EM_TINYJ = 61,
    EM_X86_64 = 62,
    EM_PDSP = 63,
    EM_PDP10 = 64,
    EM_PDP11 = 65,
    EM_FX66 = 66,
    EM_ST9PLUS = 67,
    EM_ST7 = 68,
    EM_68HC16 = 69,
    EM_68HC11 = 70,
    EM_68HC08 = 71,
    EM_68HC05 = 72,
    EM_SVX = 73,
    EM_ST19 = 74,
    EM_VAX = 75,
    EM_CRIS = 76,
    EM_JAVELIN = 77,
    EM_FIREPATH = 78,
    EM_ZSP = 79,
    EM_MMIX = 80,
    EM_HUANY = 81,
    EM_PRISM = 82,
    EM_AVR = 83,
    EM_FR30 = 84,
    EM_D10V = 85,
    EM_D30V = 86,
    EM_V850 = 87,
    EM_M32R = 88,
    EM_MN10300 = 89,
    EM_MN10200 = 90,
    EM_PJ = 91,
    EM_OPENRISC = 92,
    EM_ARC_A5 = 93,
    EM_XTENSA = 94,
    EM_VIDEOCORE = 95,
    EM_TMM_GPP = 96,
    EM_NS32K = 97,
    EM_TPC = 98,
    EM_SNP1K = 99,
    EM_ST200 = 100,
};
