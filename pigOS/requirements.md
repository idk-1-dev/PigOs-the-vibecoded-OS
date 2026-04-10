# pigOS Build Requirements
# Install these packages to build pigOS on your host system

# ========================================
# Core Build Tools (REQUIRED)
# ========================================
gcc              # C compiler (x86_64, ELF target)
make             # Build system
nasm             # Assembler for boot code (x86_64)

# ========================================
# Bootloader & ISO (REQUIRED)
# ========================================
grub-pc-bin      # GRUB2 bootloader
grub-common      # GRUB2 utilities
mtools           # MS-DOS filesystem tools (for GRUB)
xorriso          # ISO 9660 tool (create bootable ISO)

# ========================================
# Emulator (OPTIONAL - for testing)
# ========================================
qemu-system-x86  # QEMU emulator (run pigOS.iso)
  # Or: qemu-kvm for faster emulation

# Build and run:
#   make          # Build pigOS.iso
#   qemu-system-x86_64 -cdrom pigOS.iso -m 512

# ========================================
# Development Tools (OPTIONAL)
# ========================================
# For debugging kernel
# gdb              # GNU debugger
# objdump          # Disassembler
# readelf          # ELF file analyzer

# ========================================
# Dependencies Note
# ========================================
# pigOS is a custom kernel - it does NOT require:
#   - Linux headers
#   - glibc
#   - any standard library
#
# It's built from scratch with:
#   - Custom memory allocator (mem.h)
#   - Custom string functions
#   - No external dependencies

# ========================================
# Building pigOS
# ========================================
# From pigOS directory:
#   make clean    # Clean previous build
#   make          # Build kernel and ISO
#   make run      # Build and run in QEMU
#
# Output: pigOS.iso (bootable)
#         build/pig.bin (kernel ELF)
