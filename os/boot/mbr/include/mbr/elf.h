/**********************************************************
 *			elf.h  //add by visual 2016.5.16
 ***********************************************************/
// ported by sundong 2023.3.26
#ifndef _ELF_H_
#define _ELF_H_
#include "type.h"
#define EI_NIDENT 16
#define PT_LOAD 1
struct Elfdr {
  u32 e_magic;  // must equal ELF_MAGIC
  u8 e_elf[12];
  u16 e_type;       // ELF文件类型
  u16 e_machine;    //体系结构类型
  u32 e_version;    //文件版本
  u32 e_entry;      //程序入口地址
  u32 e_phoff;      //程序头表偏移量
  u32 e_shoff;      //节头表偏移
  u32 e_flags;      //与文件相关的特定处理器的标准
  u16 e_ehsize;     // ELF头部的大小
  u16 e_phentsize;  //程序头表
  u16 e_phnum;      //程序头表的数量
  u16 e_shentsize;  //程序头表项的大小
  u16 e_shnum;      //节头表表项的大小
  u16 e_shstrndx;   //段表字符串在节头表的索引
};

struct Proghdr {
  u32 p_type;    //该program 	类型
  u32 p_offset;  //该program	在文件中的偏移量
  u32 p_va;      //该program	应该放在这个线性地址
  u32 p_pa;      //该program
             //应该放在这个物理地址（对只使用物理地址的系统有效）
  u32 p_filesz;  //该program	在文件中的长度
  u32 p_memsz;  //该program	在内存中的长度（不一定和filesz相等）
  u32 p_flags;  //该program	读写权限
  u32 p_align;  //该program	对齐方式
};

struct Secthdr {
  u32 sh_name;
  u32 sh_type;
  u32 sh_flags;
  u32 sh_addr;
  u32 sh_offset;
  u32 sh_size;
  u32 sh_link;
  u32 sh_info;
  u32 sh_addralign;
  u32 sh_entsize;
};
#endif