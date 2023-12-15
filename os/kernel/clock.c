
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               clock.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "proc.h"
#include "global.h"
#include "string.h"
#include "proto.h"
#include "time.h"

int		ticks;
/*======================================================================*
                           clock_handler
 *======================================================================*/
PUBLIC void clock_handler(int irq)
{
	ticks++;
	
	/* There is two stages - in kernel intializing or in process running.
	 * Some operation shouldn't be valid in kernel intializing stage.
	 * added by xw, 18/6/1
	 */
	if(kernel_initial == 1){
		return;
	}
	
	p_proc_current->task.ticks--;
	sys_wakeup(&ticks);

	//to make syscall reenter, deleted by xw, 17/12/11
	/*
	if (k_reenter != 0) {
		return;
	}
	*/

	/*	//Moved into schedule(). xw, 18/4/21
	if (p_proc_current->task.ticks > 0) {
		//do you know why this statement is added here? p_proc_current doesn't change if schedule() below 
		//isn't called. if you know the reason, please contact me at dongxuwei@163.com. added by xw, 17/12/16
//		cr3_ready = p_proc_current->task.cr3;  //add by visual 2016.5.26		
		return;
	}
	*/

//	schedule();
//	cr3_ready = p_proc_current->task.cr3;			//add by visual 2016.4.5
//	sched();
}

/*======================================================================*
                              milli_delay
 *======================================================================*/
PUBLIC void milli_delay(int milli_sec)
{
        int t = get_ticks();

        while(((get_ticks() - t) * 1000 / HZ) < milli_sec) {}
}

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