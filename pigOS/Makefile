# pigOS Build System
CC   = gcc
NASM = nasm
LD   = ld

# ── Kernel Configuration ────────────────────────────────
-include kernel/.config

CFLAGS = -m64 -ffreestanding -fno-builtin -fno-stack-protector \
		 -fno-pic -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-80387 \
         -Wall -Wextra -O2 -std=gnu99 \
         -Wno-unused-parameter -Wno-unused-function \
         -Wno-unused-variable -Wno-type-limits \
         -Wno-misleading-indentation \
         -I. -Ilwip/src/include -Ilwip/port -Ilwip/port/arch \
         -include kernel/autoconf.h

LDFLAGS = -m elf_x86_64 -T linker.ld

# lwIP source files
LWIP_CORE = \
    lwip/src/core/init.c \
    lwip/src/core/def.c \
    lwip/src/core/mem.c \
    lwip/src/core/memp.c \
    lwip/src/core/netif.c \
    lwip/src/core/pbuf.c \
    lwip/src/core/raw.c \
    lwip/src/core/stats.c \
    lwip/src/core/sys.c \
    lwip/src/core/tcp.c \
    lwip/src/core/tcp_in.c \
    lwip/src/core/tcp_out.c \
    lwip/src/core/timeouts.c \
    lwip/src/core/udp.c \
    lwip/src/core/dns.c \
    lwip/src/core/inet_chksum.c \
    lwip/src/core/ip.c \
    lwip/src/core/ipv4/acd.c \
    lwip/src/core/ipv4/autoip.c \
    lwip/src/core/ipv4/dhcp.c \
    lwip/src/core/ipv4/etharp.c \
    lwip/src/core/ipv4/icmp.c \
    lwip/src/core/ipv4/igmp.c \
    lwip/src/core/ipv4/ip4.c \
    lwip/src/core/ipv4/ip4_addr.c \
    lwip/src/core/ipv4/ip4_frag.c \
    lwip/src/netif/ethernet.c

LWIP_OBJS = $(patsubst %.c,build/%.o,$(LWIP_CORE))

OBJECTS = build/boot.o build/kernel.o build/libc_stubs.o $(LWIP_OBJS)

all: dirs build/pig.bin pigOS.iso

dirs: kernel/autoconf.h
	@mkdir -p build/lwip/src/core/ipv4 build/lwip/src/core/ipv6 build/lwip/src/api build/lwip/src/apps build/lwip/src/netif build/lwip/src/netif/ppp

kernel/autoconf.h: kernel/.config
	@bash scripts/genconfig.sh kernel/.config kernel/autoconf.h

build/boot.o: boot/boot.asm | dirs
	$(NASM) -f elf64 boot/boot.asm -o build/boot.o

build/kernel.o: kernel/kernel.c | dirs
	$(CC) $(CFLAGS) -c kernel/kernel.c -o build/kernel.o

build/libc_stubs.o: kernel/libc_stubs.c | dirs
	$(CC) $(CFLAGS) -c kernel/libc_stubs.c -o build/libc_stubs.o

build/lwip/src/core/%.o: lwip/src/core/%.c | dirs
	$(CC) $(CFLAGS) -Dmemcpy=pig_memcpy -Dmemmove=pig_memmove -Dmemset=pig_memset -Dmemcmp=pig_memcmp -Dstrlen=pig_strlen -Dstrncmp=pig_strncmp -Dstrtol=pig_strtol -Wno-error -c $< -o $@

build/lwip/src/core/ipv4/%.o: lwip/src/core/ipv4/%.c | dirs
	$(CC) $(CFLAGS) -Dmemcpy=pig_memcpy -Dmemmove=pig_memmove -Dmemset=pig_memset -Dmemcmp=pig_memcmp -Dstrlen=pig_strlen -Dstrncmp=pig_strncmp -Dstrtol=pig_strtol -Wno-error -c $< -o $@

build/lwip/src/netif/%.o: lwip/src/netif/%.c | dirs
	$(CC) $(CFLAGS) -Dmemcpy=pig_memcpy -Dmemmove=pig_memmove -Dmemset=pig_memset -Dmemcmp=pig_memcmp -Dstrlen=pig_strlen -Dstrncmp=pig_strncmp -Dstrtol=pig_strtol -Wno-error -c $< -o $@

build/pig.bin: $(OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o build/pig.bin $(OBJECTS)
	@echo "[OK] pigKernel built: build/pig.bin"

pigOS.iso: build/pig.bin
	cp build/pig.bin iso/boot/pig.bin
	@echo "Building ISO with GRUB..."
	nix shell nixpkgs#xorriso nixpkgs#grub2 --command \
	    grub-mkrescue -o pigOS.iso --modules="multiboot all_video boot cat configfile echo fat font gfxterm linux ls memdisk minicmd normal part_msdos search serial terminal video" iso 2>/dev/null || \
	grub-mkrescue -o pigOS.iso --modules="multiboot all_video boot cat configfile echo fat font gfxterm linux ls memdisk minicmd normal part_msdos search serial terminal video" iso
	@echo "pigOS.iso ready!"

run: pigOS.iso
	qemu-system-x86_64 -cdrom pigOS.iso -m 128M -vga std -net nic,model=rtl8139 -net user,hostfwd=tcp::8080-:80,hostfwd=tcp::2222-:22 -usb -device usb-tablet

run-hostfwd: pigOS.iso
	qemu-system-x86_64 -cdrom pigOS.iso -m 128M -vga std -net nic,model=rtl8139 -net user,hostfwd=tcp::5555-:55,hostfwd=tcp::8080-:80 -usb -device usb-tablet

run-bridge: pigOS.iso
	qemu-system-x86_64 -cdrom pigOS.iso -m 128M -vga std -net bridge,br=br0 -usb -device usb-tablet

run-virtio: pigOS.iso
	qemu-system-x86_64 -cdrom pigOS.iso -m 128M -vga std -netdev user,id=net0 -device virtio-net-pci,netdev=net0,mac=52:54:00:12:34:56 -usb -device usb-tablet

run-debug: pigOS.iso
	qemu-system-x86_64 -cdrom pigOS.iso -m 128M -vga std -netdev user,id=hn0 -device rtl8139,netdev=hn0 -object filter-dump,id=f1,netdev=hn0,file=pigOS.pcap -usb -device usb-tablet

run-socket: pigOS.iso
	@echo "=== Socket-based networking ==="
	@echo "Starting QEMU with TCP socket connection..."
	@echo "Use: nc 127.0.0.1 12345 on another terminal to connect"
	qemu-system-x86_64 -cdrom pigOS.iso -m 128M -vga std -netdev socket,listen=127.0.0.1:12345,id=sock0 -device rtl8139,netdev=sock0 -usb -device usb-tablet

run-tap: pigOS.iso
	@echo "=== TAP Networking Setup ==="
	@echo "This will create a virtual network for pigOS"
	@echo "Your actual network won't be affected"
	@sudo ip link show tap0 2>/dev/null || sudo ip tuntap add dev tap0 mode tap
	@sudo ip link set tap0 up
	@sudo ip addr flush dev tap0
	@sudo ip addr add 10.0.3.1/24 dev tap0
	@echo "Host TAP IP: 10.0.3.1 (guest will be 10.0.3.15)"
	@echo ""
	@echo "Starting QEMU with TAP..."
	qemu-system-x86_64 -cdrom pigOS.iso -m 128M -vga std -netdev tap,id=net0,ifname=tap0,script=no,downscript=no -device rtl8139,netdev=net0 -usb -device usb-tablet

run-e1000: pigOS.iso
	qemu-system-x86_64 -cdrom pigOS.iso -m 128M -vga std -netdev user,id=net0 -device e1000,netdev=net0 -usb -device usb-tablet

clean:
	rm -rf build/ pigOS.iso iso/boot/pig.bin

menuconfig:
	@bash scripts/menuconfig.sh

defconfig:
	@bash scripts/genconfig.sh kernel/.config kernel/autoconf.h
	@echo "  LD   build/kernel.elf"
autoconf: kernel/.config
	@bash scripts/genconfig.sh kernel/.config kernel/autoconf.h

.PHONY: all dirs run clean menuconfig defconfig autoconf run-hostfwd run-bridge run-virtio run-debug run-socket run-tap run-e1000
