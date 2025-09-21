CC = aarch64-linux-gnu-gcc
AS = aarch64-linux-gnu-as
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

CFLAGS = -ffreestanding -nostdlib -nostartfiles -O2 -Wall -Wextra -mcpu=cortex-a53
ASFLAGS = -mcpu=cortex-a53
LDFLAGS = -T linker.ld
SOURCES_C = kernel.c vm_maps.c cpu.c crash_core.c font_data.c dtb.c security.c astral_sched.c kmalloc.c kprintf.c fs.c block_device.c lib.c
SOURCES_S = bootloader.s exceptions.s context_switch.s
OBJECTS = $(SOURCES_C:.c=.o) $(SOURCES_S:.s=.o)

TARGET = astral.elf
BINARY = astral.bin

all: $(BINARY)

$(BINARY): $(TARGET)
	$(OBJCOPY) -O binary $< $@

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) $(BINARY)

qemu: $(BINARY)
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 8 -kernel $(BINARY) -nographic -append "dtb=0x40000000"

qemugfx: $(BINARY)
	qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 8 -kernel $(BINARY) -append "dtb=0x40000000" -device virtio-gpu-pci -vnc :0


