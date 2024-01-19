#include "time.h"
#include "const.h"
#include "protect.h"
#include "proto.h"


PUBLIC void RTC_handler(){
}

void NMI_enable() {
    out_byte(0x70, in_byte(0x70) & 0x7F);
    in_byte(0x71);
}
 
void NMI_disable() {
    out_byte(0x70, in_byte(0x70) | 0x80);
    in_byte(0x71);
}

void initRTC(){
	put_irq_handler(RTC_IRQ, RTC_handler);
}

u8 readRTC(int addr){
	//*** must disable int and nmi ***
	out_byte(0x70, addr);
	return in_byte(0x71);
}

int bcd2byte(u8 x){
	return ((x>>4)&0xF)*10 + (x & 0xF);
}

void get_rtc_datetime(struct tm* time){
	disable_int();
	NMI_disable();
	time->tm_year = bcd2byte(readRTC(9))+2000-1900;
	time->tm_mon = bcd2byte(readRTC(8)) - 1;
	time->tm_mday = bcd2byte(readRTC(7));
	time->tm_hour = bcd2byte(readRTC(4));
	time->tm_min = bcd2byte(readRTC(2));
	time->tm_sec = bcd2byte(readRTC(0));
	NMI_enable();
	enable_int();
}

PUBLIC int kern_get_time(struct tm* time)
{
	get_rtc_datetime(time);
	return 0;
}

PUBLIC int do_get_time(struct tm* time)
{
	return kern_get_time(time);
}

PUBLIC int sys_get_time()
{
	return do_get_time((struct tm*)get_arg(1));
}