
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               clock.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "clock.h"
#include "proc.h"
#include "string.h"
#include "proto.h"

int		ticks;
u32 	current_timestamp;
/*======================================================================*
                           clock_handler
 *======================================================================*/
PUBLIC void clock_handler(int irq)
{
	ticks++;
	if(!(ticks % HZ)){
		current_timestamp++;
	}
	
	/* There is two stages - in kernel intializing or in process running.
	 * Some operation shouldn't be valid in kernel intializing stage.
	 * added by xw, 18/6/1
	 */
	if(kernel_initial == 1){
		return;
	}
	
	proc_update();
	wakeup(&ticks);

}

PUBLIC void init_clock(){
	ticks = 0;
	/* initialize 8253 PIT */
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8)(TIMER_FREQ / HZ));
	out_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));
	/* initialize clock-irq */
	put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
	current_timestamp = get_init_rtc_timestamp();
	enable_irq(CLOCK_IRQ);					   /* 让8259A可以接收时钟中断 */
}
/*======================================================================*
                           get_ticks		add by visual 2016.4.6
 *======================================================================*/
PUBLIC int kern_get_ticks()
{
	return ticks;
}

PUBLIC int do_get_ticks()
{
	return kern_get_ticks();
}

PUBLIC int sys_get_ticks()
{
	return do_get_ticks();
}