/* kernel/debug.c */
extern void (*krn_debug_status_cb)(const char *, ...);
extern char krn_debug_buffer[DEBUG_BUFFER_ROWS][DEBUG_BUFFER_COLS];
extern uint32_t krn_debug_buffer_gen;
extern void krn_debug_putc(char c);
extern void krn_debug_printf(const char *fmt, ...);
extern void krn_debug_assert(int expr, const char *file, unsigned line);
extern void krn_debug_beep(unsigned hz, unsigned msecs, unsigned count);
/* kernel/event.c */
extern int krn_event_ipush(event_st event);
extern int krn_event_push(event_st event);
extern int krn_event_pop(event_st *event);
extern uint16_t krn_event_count(void);
/* kernel/initrd.c */
extern void krn_initrd_init(void);
/* kernel/intr.c */
extern __attribute__((force_align_arg_pointer)) void krn_intr_handle(isr_stack_st *isr_stack);
extern void krn_intr_set_handler(uint8_t int_no, isr_handler_fn handler);
extern void krn_intr_init(void);
/* kernel/keyboard.c */
extern void krn_keyboard_init(void);
/* kernel/lock.c */
extern krn_lock_t krn_lock(void);
extern void krn_unlock(krn_lock_t lock);
/* kernel/main.c */
extern system_info_st krn_system_info;
extern void krn_main(void);
/* kernel/mboot.c */
extern void krn_mboot_dump(void);
extern void krn_mboot_init(void);
/* kernel/mem.c */
extern int krn_mem_check_a20(void);
extern void krn_mem_init(void);
/* kernel/mouse.c */
extern void krn_mouse_handle_abs_packet(int x, int y, int btn_left, int btn_right);
extern void krn_mouse_handle_rel_packet(int dx, int dy, int btn_left, int btn_right);
extern void krn_mouse_handle_uart_data(uint8_t data);
extern void krn_mouse_handle_ps2_data(uint8_t data);
extern void krn_mouse_init(void);
/* kernel/pic.c */
extern void krn_pic_init(void);
/* kernel/ps2.c */
extern uint16_t krn_ps2_read_data_with_timeout(size_t timeout);
extern uint16_t krn_ps2_read_data(void);
extern void krn_ps2_reboot(void);
extern void krn_ps2_enable_a20(void);
extern void krn_ps2_init(void);
/* kernel/rtc.c */
extern int krn_rtc_get_time(time_st *t);
extern void krn_rtc_set_time(time_st *t);
extern void krn_rtc_init(void);
/* kernel/speaker.c */
extern void krn_speaker_get_state(speaker_state_st *out);
extern void krn_speaker_play_song(const note_st *notes, void *owner);
extern void krn_speaker_play_freq(unsigned hz, void *owner);
extern void krn_speaker_pause(void *owner);
extern void krn_speaker_resume(void *owner);
extern void krn_speaker_stop(void *owner);
extern void krn_speaker_seek(void *owner, uint32_t ticks);
extern void krn_speaker_on_tick(void);
/* kernel/system.c */
extern uint32_t krn_system_get_total_mem(void);
extern uint32_t krn_system_get_used_mem(void);
extern uint32_t krn_system_get_avail_mem(void);
/* kernel/timer.c */
extern volatile uint8_t krn_timer_is_cpu_idle;
extern uint32_t krn_timer_get_msecs(void);
extern uint16_t krn_timer_get_counter_0(void);
extern uint8_t krn_timer_get_cpu_usage(void);
extern void krn_timer_init(void);
/* kernel/uart.c */
extern void krn_uart_write_data(uint8_t data);
extern int krn_uart_set_mode(int mode);
extern void krn_uart_init(void);
/* kernel/vga.c */
extern void krn_vga_set_color(int index, uint32_t rgb);
extern void krn_vga_init(void);
/* kernel/vmware.c */
extern int krn_vmware_handle_mouse_intr(void);
extern void krn_vmware_init(void);
