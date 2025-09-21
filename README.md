# astral os

a bare-metal operating system for arm64 architecture.

## features

- arm64 support
- ufs 2.2 and emmc 5.0 block device drivers
- virtual memory management (mmu) with per-task page tables
- pre-emptive scheduler with timer interrupts
- dynamic memory allocation (kmalloc)
- basic kprintf console output
- serial uart output

## build instructions

### prerequisites

install cross-compilation toolchain and iso creation tools:

```bash
sudo apt update
sudo apt install -y build-essential crossbuild-essential-arm64 grub-pc-bin xorriso
```

### compile

```bash
make clean
make
```

## running in qemu

to run the compiled kernel in qemu:

```bash
make qemu
```

for graphical output (if framebuffer is initialized):

```bash
make qemugfx
```

## debugging in qemu

to run qemu and wait for a gdb connection:

```bash
make qemu-debug
```

then, in another terminal, connect gdb:

```bash
gdb-multiarch astral.elf -ex "target remote :1234" -ex "b kernel_main" -ex "c"
```

## iso creation

to create a bootable iso (for virtualbox/qemu with grub):

```bash
mkdir -p rootfs/boot rootfs/bin rootfs/etc rootfs/dev
cp astral.bin rootfs/boot/kernel.img
mkdir -p rootfs/boot/grub
cat << EOF > rootfs/boot/grub/grub.cfg
set default="0"
set timeout="0"

menuentry "astral os" {
    insmod all_video
    insmod gzio
    insmod part_gpt
    insmod ext2
    set root=\\'(hd0,gpt1)\\\'
    linux /boot/kernel.img
}
EOF
xorriso -as mkisofs -r -f -o astral_os.iso -graft-points /boot/kernel.img=astral.bin /boot/grub/grub.cfg=rootfs/boot/grub/grub.cfg
```


