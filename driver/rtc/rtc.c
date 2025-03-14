#include <driver/rtc/rtc.h>
#include <driver/rtc/dev_ops.h>
#include <driver/acpi/acpi.h>
#include <driver/acpi/res_types.h>
#include <minios/asm.h>
#include <minios/assert.h>
#include <minios/interrupt.h>
#include <minios/dev.h>
#include <fs/devfs/devfs.h>

static int rtc0_port_io_addr = -1;

static int rtc_probe_impl(lai_nsnode_t *node, lai_state_t *state, void *arg, void *data) {
    UNUSED(node, state, arg);
    acpi_resdt_io_port_t *d = data;
    rtc0_port_io_addr = d->addr_base_min;
    register_device(DEV_MAKE_ID(DEV_CHAR_RTC, 0), &rtc_file_ops);
    return ACPI_VISIT_STOP;
}

int rtc_probe() {
    acpi_find_device("PNP0B00", rtc_probe_impl, NULL);
    return rtc0_port_io_addr == -1 ? -1 : 0;
}

static uint8_t rtc_io_read(int addr) {
    outb(rtc0_port_io_addr + RTC_REG_ADDR, addr);
    return inb(rtc0_port_io_addr + RTC_REG_DATA);
}

static int bcd2byte(uint8_t x) {
    return ((x >> 4) & 0xf) * 10 + (x & 0xf);
}

void rtc_get_datetime(struct tm *tm) {
    assert(rtc0_port_io_addr != -1);
    int year = 0;
    int mon = 0;
    int day = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;
    disable_int_begin();
    do {
        year = rtc_io_read(0x9);
        mon = rtc_io_read(0x8);
        day = rtc_io_read(0x7);
        hour = rtc_io_read(0x4);
        min = rtc_io_read(0x2);
        sec = rtc_io_read(0x0);
    } while (sec != rtc_io_read(0x0));
    if (!(rtc_io_read(0xb) & 0x4)) {
        year = bcd2byte(year);
        mon = bcd2byte(mon);
        day = bcd2byte(day);
        hour = bcd2byte(hour);
        min = bcd2byte(min);
        sec = bcd2byte(sec);
    }
    disable_int_end();
    year += 1900;
    if (year < 1970) { year += 100; }
    tm->tm_year = year - 1900;
    tm->tm_mon = mon - 1;
    tm->tm_mday = day;
    tm->tm_hour = hour;
    tm->tm_min = min;
    tm->tm_sec = sec;
    tm->__tm_gmtoff = RTC_TIMEZONE * 3600;
}
