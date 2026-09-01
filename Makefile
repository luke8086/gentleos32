CC              := gcc
LD              := ld
NASM            := nasm
OBJCOPY         := objcopy

BASEDIR         := .
BUILDDIR        := $(BASEDIR)/build

KERNEL_HIMEM_ELF    := $(BUILDDIR)/kernel-himem.elf
KERNEL_HIMEM_BIN    := gentleos.bin
KERNEL_LOMEM_ELF    := $(BUILDDIR)/kernel-lomem.elf
KERNEL_LOMEM_BIN    := $(BUILDDIR)/kernel-lomem.bin

DISK_IMAGE      := gentleos32-disk.img
GRUB_IMAGE      := gentleos32-grub.img
WEB_IMAGE       := gentleos32-web.img
EMU_PAGE        := gentleos32-emu.html
DISK_FS_OFFSET  := 1048576

KERNEL_HIMEM_LD := $(BASEDIR)/misc/kernel-himem.ld
KERNEL_LOMEM_LD := $(BASEDIR)/misc/kernel-lomem.ld

KERNEL_ASFLAGS  :=

KERNEL_CPPFLAGS :=
KERNEL_CFLAGS   := -std=c11 -m32 -march=i386 -O2 \
                   -ffreestanding -fno-stack-protector -fno-pic \
                   -Wall -Wextra -pedantic \
                   -I$(BASEDIR)/include \
                   $(KERNEL_CPPFLAGS)

KERNEL_LDFLAGS  := -m elf_i386 -nostdlib -z nodefaultlib \
                   -z noexecstack --no-warn-rwx-segments \
                   --orphan-handling=warn

KERNEL_SUBDIRS  := lib kernel gui feat apps
KERNEL_C_SRCS   := $(foreach d,$(KERNEL_SUBDIRS),$(wildcard $(d)/*.c))
KERNEL_S_SRCS   := $(foreach d,$(KERNEL_SUBDIRS),$(wildcard $(d)/*.s))
KERNEL_SRCS     := $(KERNEL_C_SRCS) $(KERNEL_S_SRCS)
KERNEL_OBJS     := $(patsubst %.c,$(BUILDDIR)/%.o,$(KERNEL_C_SRCS)) \
                   $(patsubst %.s,$(BUILDDIR)/%.o,$(KERNEL_S_SRCS)) \
                   $(BUILDDIR)/data.o
KERNEL_DEPS     := $(KERNEL_OBJS:.o=.d)


BOOT_CFLAGS     := -std=c11 -m16 -march=i386 -Os \
                   -ffreestanding -fno-stack-protector -fno-pic \
                   -fno-asynchronous-unwind-tables \
                   -Wall -Wextra -pedantic \
                   -I$(BASEDIR)/include

BOOT_LDFLAGS    := -m elf_i386 -nostdlib -z nodefaultlib \
                   -z noexecstack --no-warn-rwx-segments \
                   --orphan-handling=warn

BOOT_SUBDIRS    := boot
BOOT_LD         := misc/boot.ld
BOOT_OBJS       := $(BUILDDIR)/boot/boot_a.o $(BUILDDIR)/boot/boot_c.o
BOOT_DEPS       := $(BOOT_OBJS:.o=.d)
BOOT_ELF        := $(BUILDDIR)/boot/boot.elf
BOOT_BIN        := $(BUILDDIR)/boot.bin

SONG_SRCS       := $(wildcard assets/songs/*.musicxml)
SONG_OBJS       := $(patsubst %.musicxml,$(BUILDDIR)/%.spk,$(SONG_SRCS))

INITRD_OBJS     := $(BASEDIR)/vendor/misc/Dachshund.png \
                   $(BASEDIR)/vendor/misc/Mondrian.png

OBJDIRS := $(addprefix $(BUILDDIR)/,$(KERNEL_SUBDIRS)) \
           $(addprefix $(BUILDDIR)/,$(BOOT_SUBDIRS)) \
           $(BUILDDIR)/assets/songs

all: disks

disks: $(KERNEL_HIMEM_BIN) $(KERNEL_LOMEM_BIN) $(BOOT_BIN)
	zcat $(BASEDIR)/misc/grub-disk.img.gz > $(GRUB_IMAGE)
	mcopy -D o -i $(GRUB_IMAGE)@@$(DISK_FS_OFFSET) $(KERNEL_HIMEM_BIN) ::
	mcopy -D o -i $(GRUB_IMAGE)@@$(DISK_FS_OFFSET) $(BASEDIR)/misc/grub.sample.cfg ::boot/grub/grub.cfg
	[ -f $(BASEDIR)/misc/grub.cfg ] && mcopy -D o -i $(GRUB_IMAGE)@@$(DISK_FS_OFFSET) $(BASEDIR)/misc/grub.cfg ::boot/grub/grub.cfg || true
	./tools/mkinitrd.py $(INITRD_OBJS) -o $(BUILDDIR)/gentleos-grub.rd --disk-image $(GRUB_IMAGE)

	./tools/mkdisk.pl $(BOOT_BIN) $(KERNEL_LOMEM_BIN) $(DISK_IMAGE)

	./tools/mkdisk.pl $(BOOT_BIN) $(KERNEL_LOMEM_BIN) $(WEB_IMAGE) no-menu uart-debug
	./tools/mkinitrd.py $(INITRD_OBJS) -o $(BUILDDIR)/gentleos-web.rd --disk-image $(WEB_IMAGE)
	./tools/padfile.py $(WEB_IMAGE) 1048576

	./tools/mkemu.py $(WEB_IMAGE) $(EMU_PAGE)

clean:
	rm -rf $(BUILDDIR) $(KERNEL_HIMEM_BIN) $(DISK_IMAGE) $(GRUB_IMAGE) $(WEB_IMAGE) $(EMU_PAGE)

$(OBJDIRS):
	@mkdir -p $@

$(KERNEL_HIMEM_ELF): $(KERNEL_OBJS) $(KERNEL_HIMEM_LD)
	$(LD) $(KERNEL_LDFLAGS) -T$(KERNEL_HIMEM_LD) $(KERNEL_OBJS) -o $@

$(KERNEL_LOMEM_ELF): $(KERNEL_OBJS) $(KERNEL_LOMEM_LD)
	$(LD) $(KERNEL_LDFLAGS) -T$(KERNEL_LOMEM_LD) $(KERNEL_OBJS) -o $@

$(KERNEL_HIMEM_BIN): $(KERNEL_HIMEM_ELF)
	$(OBJCOPY) -O binary $< $@

$(KERNEL_LOMEM_BIN): $(KERNEL_LOMEM_ELF)
	$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/assets/songs/%.spk: assets/songs/%.musicxml
	python3 tools/mkspk.py -i $< -o $@

$(BUILDDIR)/data.o: $(BUILDDIR)/data.c
	$(CC) $(KERNEL_CFLAGS) -MMD -MP -c $< -o $@

ALWAYS_REBUILD:

$(BUILDDIR)/data.c: $(SONG_OBJS) ALWAYS_REBUILD | $(OBJDIRS)
	python3 ./tools/mkdata.py

$(BUILDDIR)/%.o: %.c | $(OBJDIRS)
	$(CC) $(KERNEL_CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.o: %.s | $(OBJDIRS)
	$(NASM) $(KERNEL_ASFLAGS) -f elf32 $< -o $@

$(BUILDDIR)/boot/boot_c.o: boot/boot_c.c | $(OBJDIRS)
	$(CC) $(BOOT_CFLAGS) -MMD -MP -c $< -o $@

$(BOOT_ELF): $(BOOT_OBJS) $(BOOT_LD)
	$(LD) $(BOOT_LDFLAGS) -T$(BOOT_LD) $(BOOT_OBJS) -o $@

$(BOOT_BIN): $(BOOT_ELF)
	$(OBJCOPY) -O binary $< $@

print:
	@echo "KERNEL_SUBDIRS=$(KERNEL_SUBDIRS)"
	@echo "KERNEL_SRCS=$(KERNEL_SRCS)"
	@echo "KERNEL_OBJS=$(KERNEL_OBJS)"

.PHONY: all clean kernel print check-config

# Include auto-generated dependency files if they exist
-include $(KERNEL_DEPS) $(BOOT_DEPS)
