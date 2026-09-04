/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: system.c - System information
 */

#include <kernel.h>

global uint32_t
krn_system_get_total_mem(void)
{
    system_info_st *si = &krn_system_info;

    if (!si->mem_fields_valid) {
        return 0;
    }

    return (si->mem_lower + si->mem_upper) << 10;
}

global uint32_t
krn_system_get_used_mem(void)
{
    uint32_t static_mem = ((uint32_t)&krn_link_end - (uint32_t)&krn_link_start);
    uint32_t initrd_mem = krn_system_info.initrd_size;
    uint32_t heap_mem = heap_get_used_mem();

    return static_mem + initrd_mem + heap_mem;
}

global uint32_t
krn_system_get_avail_mem(void)
{
    return heap_get_avail_mem();
}
