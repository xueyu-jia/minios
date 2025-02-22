; memory layout
BaseOfMBR        equ 0x0000
BaseOfBoot       equ 0x0000
BaseOfBootBuffer equ BaseOfBoot
BaseOfLoader     equ 0x9000

SizeOfSector     equ 512
SizeOfBootBuffer equ SizeOfSector
SizeOfBPB        equ 90
SizeOfBoot       equ SizeOfSector - SizeOfBPB - 2

OffsetOfMBR        equ 0x0800
OffsetOfBoot       equ 0x7c00
OffsetOfBPB        equ OffsetOfBoot
OffsetOfBootBuffer equ OffsetOfBoot + SizeOfSector
OffsetOfBootMain   equ OffsetOfBPB + SizeOfBPB
OffsetOfLoader     equ 0x0400

PhyPageDirBase   equ 0x1ff000 ; base addr of page directory [2M-4K]
PhyPageTableBase equ 0x200000 ; base addr of page table [2M]

PhyLoaderBase  equ BaseOfLoader << 4
