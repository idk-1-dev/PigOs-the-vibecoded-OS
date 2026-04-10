pigOS boot/ - FLARE Bootloader
==============================

boot.asm: x86_64 Assembly Bootstrap
------------------------------------
- Sets up PML4 -> PDPT -> PD page tables (2MB pages)
- Enables PAE, EFER.LME, paging
- Loads 64-bit GDT, far-jumps to long mode
- Calls kernel_main(magic, mb_info) with Multiboot info
- 64KB stack in BSS section
- Physical mode: 1MB loading address

poug.asm: POUG (PigOS Ultimate GRUB) Bootloader
-----------------------------------------
- Advanced bootloader stages
- Verbose boot output
- Modular configuration

stage2.asm: Second Stage Loader
----------------------------
- Additional boot stages if needed

GRUB Configuration
------------------
- Uses GRUB2 multiboot
- grub.cfg: Hidden menu with 5-second countdown
- Menu entry: "pigOS" -> multiboot /boot/pig.bin

Files
-----
boot.asm      - Assembly bootstrap code
poug.asm     - POUG bootloader
stage2.asm    - Second stage loader
pig.bin       - Compiled kernel ELF (linked at 1MB)
grub.cfg      - GRUB menu configuration

Boot Process
------------
1. QEMU loads GRUB from ISO
2. GRUB finds /boot/pig.bin (multiboot compliant)
3. GRUB jumps to kernel entry point
4. boot.asm sets up long mode, page tables
5. kernel_main() runs with Multiboot info
6. poing init starts, shell launches

Building Boot Sector
------------------
The Makefile handles building:
  make         - builds boot.o and kernel.o
  make pig.bin - links into kernel binary
  make iso    - creates bootable ISO