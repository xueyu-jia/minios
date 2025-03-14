#include <minios/clock.h>
#include <minios/time.h>
#include <minios/proc.h>
#include <minios/kstate.h>
#include <minios/asm.h>
#include <minios/sched.h>
#include <minios/interrupt.h>
#include <minios/console.h>
#include <minios/regs.h>
#include <driver/rtc/rtc.h>
#include <klib/stddef.h>

//! 8253/8254 PIT (Programmable Interval Timer)
//! I/O port for timer channel 0
#define TIMER0 0x40
//! I/O port for timer mode control
#define TIMER_MODE 0x43
//! 00-11-010-0 : Counter0 - LSB then MSB - rate generator - binary
#define RATE_GENERATOR 0x34
//! clock frequency for timer in PC and AT
#define TIMER_FREQ 1193182L

int ticks = 0;
u32 current_timestamp = 0;
double tsc_ns_freq = 0.0;

static u64 system_up_tsc = 0;

static void measure_tsc_freq() {
    //! TODO: handle variant tsc frequency
    static u64 tsc_seq[2] = {};
    static int entry_times = 0;
    const int measure_tick_dur = 10;
    ++entry_times;
    if (tsc_seq[1] != 0) {
        return;
    } else if (tsc_seq[0] == 0) {
        tsc_seq[0] = rdtsc();
    } else if (entry_times == measure_tick_dur) {
        tsc_seq[1] = rdtsc();
        if (system_up_tsc == 0) {
            const u64 tsc_diff = tsc_seq[1] - tsc_seq[0];
            tsc_ns_freq = tsc_diff / 1e9 / measure_tick_dur * HZ;
            system_up_tsc = tsc_seq[0];
        }
    }
}

void clock_handler(int irq) {
    UNUSED(irq);

    measure_tsc_freq();

    ticks++;
    if (ticks % HZ == 0) { current_timestamp++; }

    /* There is two stages - in kernel intializing or in process running.
     * Some operation shouldn't be valid in kernel intializing stage.
     * added by xw, 18/6/1
     */
    if (kstate_on_init) { return; }

    proc_update();
    wakeup(&ticks);
}

void init_clock() {
    ticks = 0;
    /* initialize 8253 PIT */
    outb(TIMER_MODE, RATE_GENERATOR);
    outb(TIMER0, (u8)(TIMER_FREQ / HZ));
    outb(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));
    /* initialize clock-irq */
    put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
    {
        struct tm tm = {};
        rtc_get_datetime(&tm);
        current_timestamp = mktime(&tm);
    }
    enable_irq(CLOCK_IRQ); /* 让8259A可以接收时钟中断 */
}

void early_clock_sync() {
    write_eflags(read_eflags() | EFLAGS_ID);
    kprintf("info: support cpuid: %d\n", !!(read_eflags() & EFLAGS_ID));
    kprintf("info: support rdtsc: %d\n", !!(cpuid_edx(0x1) & BIT(4)));
    kprintf("info: max extended function: %#x\n", cpuid_eax(0x80000000));
    kprintf("info: support constant tsc: %d\n", !!(cpuid_edx(0x80000007) & BIT(7)));

    enable_int();
    while (tsc_ns_freq == 0.0) {}
    disable_int();
}

int kern_getticks() {
    return ticks;
}

clock_t kern_clock() {
    const u64 sys_tsc_dur = rdtsc() - system_up_tsc;
    const double sys_dur_ns = sys_tsc_dur / tsc_ns_freq;
    return (clock_t)(sys_dur_ns / (1e9 / CLOCKS_PER_SEC));
}
