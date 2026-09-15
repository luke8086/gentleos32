/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: initrd.c - Support for a simple initial RAM disk
 */

#include <kernel.h>

enum {
    INITRD_VERSION = 2,
};

typedef struct {
    char magic[4];
    uint32_t version;
    uint32_t count;
} __attribute__((packed)) initrd_header_st;

global void
krn_initrd_init(void)
{
    system_info_st *si = &krn_system_info;
    initrd_header_st *header;
    file_st *files;
    size_t i;

    krn_debug_printf("Initializing initrd... ");

    if (si->initrd_start == 0 || si->initrd_size < sizeof(initrd_header_st)) {
        krn_debug_printf("not found\n");
        return;
    }

    header = (initrd_header_st *)si->initrd_start;
    files = (file_st *)((uint32_t)header + sizeof(initrd_header_st));

    if (strncmp(header->magic, "IRD1", 4) != 0) {
        krn_debug_printf("invalid format\n");
        return;
    }

    if (header->version != INITRD_VERSION) {
        krn_debug_printf("incompatible version\n");
        return;
    }

    krn_debug_printf("found %u files\n", header->count);

    for (i = 0; i < header->count; ++i) {
        files[i].addr = (void *)((uint32_t)files[i].addr + (uint32_t)header);
        files[i].name[sizeof(files[i].name) - 1] = 0; /* Just in case */
    }

    si->initrd_files_count = header->count;
    si->initrd_files = files;
}
