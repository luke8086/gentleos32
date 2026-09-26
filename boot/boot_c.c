/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: boot_c.c - Combined 2-stage bootloader, C parts
 */

#include <stdint.h>

#include <boot.h>

extern mboot_info_st mboot_info;
extern uint16_t stage2_sectors;
extern uint16_t kernel_sectors;
extern uint16_t initrd_sectors;
extern uint16_t boot_flags;
extern uint8_t boot_drive_index;

extern uint32_t get_elapsed_ticks(void);
extern uint16_t get_far_word(uint16_t seg, uint16_t ofs);
extern int has_key(void);
extern int get_key(void);
extern void print_char(char c);
extern void print_str(const char *s);
extern void print_ushort(uint16_t n);
extern void safe_load_remaining_sectors_c(uint16_t dest_seg, uint16_t lba, uint16_t count);
extern void stop_floppy_motor(void);
extern void copy_ext_mem(uint32_t dest, uint32_t src, uint16_t words);
extern int vbe_load_ctrl_info(vbe_ctrl_info_st *buf);
extern int vbe_load_mode_info(uint16_t mode, vbe_mode_info_st *buf);
extern int vbe_set_mode(uint16_t mode);

typedef struct {
    uint16_t number;
    uint16_t width;
    uint16_t height;
    uint16_t pitch;
    unsigned int fb_addr;
} vbe_mode_st;

enum {
    MENU_WAIT_TICKS = 16,
    VBE_MODES_MAX = 8,
    VBE_MODE_NUMBERS_MAX = 255,
};

enum {
    STAGE2_START_LBA = 2,
    KERNEL_DEST = 0x10000,
    INITRD_DEST = 0x100000,
};

enum {
    BOOT_FLAG_NO_MENU    = 1 << 0,
    BOOT_FLAG_UART_DEBUG = 1 << 1,
};

static const char *UART_CMDLINES[UART_MODE_COUNT] = {
    "",
    "uart=mouse",
    "uart=debug",
};

/* Use arbitrary locations in the available memory for the data */
static vbe_ctrl_info_st *vbe_ctrl_info = (vbe_ctrl_info_st *)0x1000;
static uint16_t *vbe_mode_numbers = (uint16_t *)0x1200;
static vbe_mode_info_st *vbe_mode_info = (vbe_mode_info_st *)0x1400;
static vbe_mode_st *vbe_modes = (vbe_mode_st *)0x1600;

static mboot_mod_st mboot_mod;

static struct {
    int uart_mode;
    int video_mode;
} kernel_config = { 0 };

static void
load_kernel(void)
{
    safe_load_remaining_sectors_c(KERNEL_DEST >> 4, STAGE2_START_LBA + stage2_sectors, kernel_sectors);
}

static int
initrd_fits_upper_mem(void)
{
    uint32_t initrd_size = (uint32_t)initrd_sectors * 512;
    uint32_t upper_mem_size = mboot_info.mem_upper * 1024;

    return initrd_size <= upper_mem_size;
}

static void
load_initrd(void)
{
    uint16_t lba = STAGE2_START_LBA + stage2_sectors + kernel_sectors;
    uint16_t remaining = initrd_sectors;
    uint32_t dest = INITRD_DEST;
    uint16_t sectors;

    while (remaining > 0) {
        sectors = remaining < 128 ? remaining : 128;

        safe_load_remaining_sectors_c(KERNEL_DEST >> 4, lba, sectors);
        copy_ext_mem(dest, KERNEL_DEST, sectors * 512 / 2);

        lba += sectors;
        dest += sectors * 512;
        remaining -= sectors;
    }

    mboot_mod.mod_start = INITRD_DEST;
    mboot_mod.mod_end = INITRD_DEST + initrd_sectors * 512;

    mboot_info.flags |= MBOOT_FLAG_MODS;
    mboot_info.mods_count = 1;
    mboot_info.mods_addr = (uint32_t)&mboot_mod;
}

static int
select_menu_item(int count)
{
    int key, ret;

    print_str("\r\nSelected option: ");

    while (1) {
        key = get_key();

        if (key == '\r') {
            key = '0';
        }

        if (key >= '0' && key < '0' + count) {
            ret = key - '0';
            print_char(key);
            break;
        }
    }

    print_str("\r\n");

    return ret;
}

static int
load_vbe_modes(void)
{
    uint32_t addr;
    uint16_t *vmn = vbe_mode_numbers;
    vbe_mode_info_st *vmi = vbe_mode_info;
    vbe_ctrl_info_st *vci = vbe_ctrl_info;
    vbe_mode_st *mode;
    int i, count = 0;

    if (!vbe_load_ctrl_info(vci)) {
        return 0;
    }

    for (i = 0; i < VBE_MODE_NUMBERS_MAX; ++i) {
        addr = (vci->mode_list.segment << 4) + vci->mode_list.offset + i * 2;

        vmn[i] = get_far_word(addr >> 4, addr & 0xf);

        if (vmn[i] == 0xffff) {
            break;
        }
    }

    vmn[VBE_MODE_NUMBERS_MAX] = 0xffff;

    for (i = 0; vmn[i] != 0xffff; ++i) {
        if (!vbe_load_mode_info(vmn[i], vmi)) {
            continue;
        }

        /* Keep only supported modes with 8-bit packed-pixel linear framebuffer */
        if ((vmi->attrs & 0x81) != 0x81 || vmi->memory_model != 0x04 || vmi->bpp != 8) {
            continue;
        }

        /* Keep only modes of supported size */
        if (vmi->width < GUI_MIN_WIDTH || vmi->height < GUI_MIN_HEIGHT) {
            continue;
        }

        mode = &vbe_modes[count];
        mode->number = vmn[i];
        mode->width = vmi->width;
        mode->height = vmi->height;
        mode->pitch = (vci->version >= 0x0300 && vmi->lin_pitch != 0)
            ? vmi->lin_pitch : vmi->pitch;
        mode->fb_addr = vmi->fb_addr;

        ++count;

        if (count >= VBE_MODES_MAX) {
            break;
        }
    }

    return count;
}

static void
enable_vbe_mode(int index)
{
    vbe_mode_st *mode = &vbe_modes[index];

    if (!vbe_set_mode(mode->number)) {
        print_str("\r\nFailed to enable the selected video mode\r\n");
        return;
    }

    mboot_info.flags |= MBOOT_FLAG_FRAMEBUFFER;
    mboot_info.fb_addr = (uint8_t *)mode->fb_addr;
    mboot_info.fb_pitch = mode->pitch;
    mboot_info.fb_width = mode->width;
    mboot_info.fb_height = mode->height;
    mboot_info.fb_bpp = 8;

    return;
}

static void
show_vbe_menu(void)
{
    int i, vbe_mode_count;

    vbe_mode_count = load_vbe_modes();

    if (vbe_mode_count == 0) {
        print_str("\r\nNo supported VESA modes found, using 640x480x16\r\n");
        return;
    }

    print_str("\r\nAvailable video modes:\r\n\r\n");
    print_str("0: 640x480x16 (default)\r\n");

    for (i = 0; i < vbe_mode_count; ++i) {
        print_char('1' + i);
        print_str(": ");
        print_ushort(vbe_modes[i].width);
        print_char('x');
        print_ushort(vbe_modes[i].height);
        print_str("x256\r\n");
    }

    kernel_config.video_mode = select_menu_item(vbe_mode_count + 1);
}

static void
show_uart_menu(void)
{
    print_str("\r\nSerial port mode:\r\n\r\n");

    print_str("0: none (default)\r\n");
    print_str("1: mouse\r\n");
    print_str("2: debug\r\n");

    kernel_config.uart_mode = select_menu_item(3);
}

static void
show_boot_menu(void)
{
    show_vbe_menu();
    show_uart_menu();
}

void
stage2_cmain(void)
{
    int key, initrd_skipped = 0;

    print_str("\r\nLoading GentleOS.");

    if (!(boot_flags & BOOT_FLAG_NO_MENU)) {
        print_str(" Press 'm' to show boot menu.");
    }

    if (initrd_sectors > 0) {
        if (initrd_fits_upper_mem()) {
            /* Must be called before load_kernel since it temporarily uses the same memory */
            load_initrd();
        } else {
            initrd_skipped = 1;
        }
    }

    load_kernel();

    if (boot_drive_index == 0 || boot_drive_index == 1) {
        stop_floppy_motor();
    }

    print_str("\r\n");

    if (initrd_skipped) {
        print_str("Not enough upper memory for initrd, skipped\r\n");
    }

    if (boot_flags & BOOT_FLAG_UART_DEBUG) {
        kernel_config.uart_mode = UART_MODE_DEBUG;
    }

    if (!(boot_flags & BOOT_FLAG_NO_MENU)) {
        do {
            key = has_key() ? get_key() : 0;

            if (key == 'm' || key == 'M') {
                show_boot_menu();
                break;
            }
        } while (has_key() || get_elapsed_ticks() < MENU_WAIT_TICKS);
    }

    mboot_info.flags |= MBOOT_FLAG_CMDLINE;
    mboot_info.cmdline = UART_CMDLINES[kernel_config.uart_mode];

    if (kernel_config.video_mode > 0) {
        enable_vbe_mode(kernel_config.video_mode - 1);
    }
}
