#include <kernel/type.h>
#include <kernel/const.h>
#include <klib/string.h>
#include <kernel/proto.h>
#include <kernel/tty.h>
#include <kernel/console.h>
#include <kernel/clock.h>
#include <klib/spinlock.h>
#include <kernel/vga.h>

int		disp_pos;
CONSOLE     console_table[NR_CONSOLES];
SPIN_LOCK video_mem_lock; // 用于 disp调用与tty_write互斥,内核初始化完成后生效
#define __TTY_DEBUG__

/* local routines */
// PRIVATE void	vga_set_cursor(unsigned int position);
// PRIVATE void	vga_set_video_start_addr(u32 addr);
PRIVATE void	flush(CONSOLE* con);
PRIVATE	void	w_copy(unsigned int dst, const unsigned int src, int size);
PRIVATE void	clear_screen(int pos, int len);
// PUBLIC int _disp_color_str(char * info, int color, int pos);// return new pos
/*****************************************************************************
 *                                init_screen
 *****************************************************************************/
/**
 * Initialize the console of a certain tty.
 *
 * @param tty  Whose console is to be initialized.
 *****************************************************************************/
PUBLIC void init_screen(TTY* tty)
{
	int nr_tty = tty - tty_table;

	tty->console = console_table + nr_tty;

	/*
	 * NOTE:
	 *   variables related to `position' and `size' below are
	 *   in WORDs, but not in BYTEs.
	 */
	int v_mem_size = V_MEM_SIZE >> 1; /* size of Video Memory */
	int size_per_con = (v_mem_size / NR_CONSOLES)/80*80;
	tty->console->orig = nr_tty * size_per_con;
	tty->console->con_size = size_per_con / SCR_WIDTH * SCR_WIDTH;
	tty->console->cursor = tty->console->crtc_start = tty->console->orig;
	tty->console->is_full = 0;
	tty->console->current_line = 0;
	//disp_int(tty->console->cursor);

	if(nr_tty==0){
		tty->console->cursor = disp_pos / 2;
	}
	const char prompt[] = "[TTY #?]\n";

	const char * p = prompt;
	for (; *p; p++){
		//disp_str("a");
		//tty->console->cursor = disp_pos / 2;
		out_char(tty->console, *p == '?' ? nr_tty + '0' : *p);
	}

	vga_set_cursor(tty->console->cursor);
}


/*****************************************************************************
 *                                out_char
 *****************************************************************************/
/**
 * Print a char in a certain console.
 *
 * @param con  The console to which the char is printed.
 * @param ch   The char to print.
 *****************************************************************************/

PUBLIC void out_char(CONSOLE* con, char ch)
{
	// acquire(&video_mem_lock);
	disable_int();


	//u8* pch = (u8*)(V_MEM_BASE + con->cursor * 2);
	//unsigned int pos = V_MEM_BASE+con->cursor*2;



	//assert(con->cursor - con->orig < con->con_size);

	/*
	 * calculate the coordinate of cursor in current console (not in
	 * current screen)
	 */
	int cursor_x = (con->cursor - con->orig) % SCR_WIDTH;
	int cursor_y = (con->cursor - con->orig) / SCR_WIDTH;

	switch(ch) {
	case '\n':
		clear_screen(con->cursor, SCR_WIDTH - cursor_x);
		con->cursor = con->orig + SCR_WIDTH * (cursor_y + 1);
		break;
	case '\b':
		if (con->cursor > con->orig) {
			con->cursor--;
			//*(pch - 2) = ' ';
			//*(pch - 1) = DEFAULT_CHAR_COLOR;
			// disp_pos = con->cursor*2;
			// write_char(' ', con->cursor*2);
			vga_write_char(' ', WHITE_CHAR, con->cursor);
		}
		break;
	default:
		//*pch++ = ch;
		//*pch++ = DEFAULT_CHAR_COLOR;
		// disp_pos = con->cursor*2;
		// write_char(ch, con->cursor*2);
		con->cursor = vga_write_char(ch, WHITE_CHAR, con->cursor);

		break;
	}



	cursor_x = (con->cursor - con->orig) % SCR_WIDTH;
	cursor_y = (con->cursor - con->orig) / SCR_WIDTH;
	if (con->cursor - con->orig >= con->con_size) {
		int cp_orig = con->orig + (cursor_y + 1) * SCR_WIDTH - SCR_SIZE;
		w_copy(con->orig, cp_orig, SCR_SIZE - SCR_WIDTH);
		con->crtc_start = con->orig;
		con->cursor = con->orig + (SCR_SIZE - SCR_WIDTH) + cursor_x;
		clear_screen(con->cursor, SCR_WIDTH - cursor_x);
		if (!con->is_full)
			con->is_full = 1;
	}

	//assert(con->cursor - con->orig < con->con_size);

	while (con->cursor >= con->crtc_start + SCR_SIZE ||
	       con->cursor < con->crtc_start) {
		scroll_screen(con, SCR_UP);

		clear_screen(con->cursor, SCR_WIDTH - cursor_x);
	}
	// if(con == console_table){
	// 	disp_pos = con->cursor*2;
	// }
	flush(con);

	enable_int();
	// release(&video_mem_lock);
}


/*****************************************************************************
 *                                clear_screen
 *****************************************************************************/
/**
 * Write whitespaces to the screen.
 *
 * @param pos  Write from here.
 * @param len  How many whitespaces will be written.
 *****************************************************************************/
PRIVATE void clear_screen(int pos, int len)
{
	u8 * pch = (u8*)(K_PHY2LIN(V_MEM_BASE) + pos * 2);
	while (--len >= 0) {
		*pch++ = ' ';
		*pch++ = DEFAULT_CHAR_COLOR;
	}
}

/*****************************************************************************
 *                            is_current_console
 *****************************************************************************/
/**
 * Uses `nr_current_console' to determine if a console is the current one.
 *
 * @param con   Ptr to console.
 *
 * @return   TRUE if con is the current console.
 *****************************************************************************/
PUBLIC int is_current_console(CONSOLE* con)
{
	return (con == &console_table[current_console]);
}


/*****************************************************************************
 *                                select_console
 *****************************************************************************/
/**
 * Select a console as the current.
 *
 * @param nr_console   Console nr, range in [0, NR_CONSOLES-1].
 *****************************************************************************/
PUBLIC void select_console(int nr_console)
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) return;

	flush(&console_table[current_console = nr_console]);
}


/*****************************************************************************
 *                                scroll_screen
 *****************************************************************************/
/**
 * Scroll the screen.
 *
 * Note that scrolling UP means the content of the screen will go upwards, so
 * that the user can see lines below the bottom. Similarly scrolling DOWN means
 * the content of the screen will go downwards so that the user can see lines
 * above the top.
 *
 * When there is no line below the bottom of the screen, scrolling UP takes no
 * effects; when there is no line above the top of the screen, scrolling DOWN
 * takes no effects.
 *
 * @param con   The console whose screen is to be scrolled.
 * @param dir   SCR_UP : scroll the screen upwards;
 *              SCR_DN : scroll the screen downwards
 *****************************************************************************/
PUBLIC void scroll_screen(CONSOLE* con, int dir)
{
	/*
	 * variables below are all in-console-offsets (based on con->orig)
	 */
	int oldest; /* addr of the oldest available line in the console */
	int newest; /* .... .. ... latest ......... .... .. ... ....... */
	int scr_top;/* position of the top of current screen */

	newest = (con->cursor - con->orig) / SCR_WIDTH * SCR_WIDTH;
	oldest = con->is_full ? (newest + SCR_WIDTH) % con->con_size : 0;
	scr_top = con->crtc_start - con->orig;

	if (dir == SCR_DN) {
		if (!con->is_full && scr_top > 0) {
			con->crtc_start -= SCR_WIDTH;
		}
		else if (con->is_full && scr_top != oldest) {
			if (con->cursor - con->orig >= con->con_size - SCR_SIZE) {
				if (con->crtc_start != con->orig)
					con->crtc_start -= SCR_WIDTH;
			}
			else if (con->crtc_start == con->orig) {
				scr_top = con->con_size - SCR_SIZE;
				con->crtc_start = con->orig + scr_top;
			}
			else {
				con->crtc_start -= SCR_WIDTH;
			}
		}
	}
	else if (dir == SCR_UP) {
		if (!con->is_full && newest >= scr_top + SCR_SIZE) {
			con->crtc_start += SCR_WIDTH;
		}
		else if (con->is_full && scr_top + SCR_SIZE - SCR_WIDTH != newest) {
			if (scr_top + SCR_SIZE == con->con_size)
				con->crtc_start = con->orig;
			else
				con->crtc_start += SCR_WIDTH;
		}
	}
	else {
		//assert(dir == SCR_DN || dir == SCR_UP);
	}

	flush(con);
}

PUBLIC void reset_screen(CONSOLE* con) {
	int latest_start = (con->cursor - con->orig) / SCR_WIDTH * SCR_WIDTH + SCR_WIDTH - SCR_SIZE;
	if(latest_start < 0){
		latest_start = 0;
	}
	con->crtc_start = con->orig + latest_start;
	flush(con);
}

/*****************************************************************************
 *                                flush
 *****************************************************************************/
/**
 * Set the cursor and starting address of a console by writing the
 * CRT Controller Registers.
 *
 * @param con  The console to be set.
 *****************************************************************************/
PRIVATE void flush(CONSOLE* con)
{
	if (is_current_console(con)) {
		vga_set_cursor(con->cursor);
		vga_set_video_start_addr(con->crtc_start);
	}
/*
#ifdef __TTY_DEBUG__
	int lineno = 0;
	for (lineno = 0; lineno < con->con_size / SCR_WIDTH; lineno++) {
		u8 * pch = (u8*)(V_MEM_BASE +
				   (con->orig + (lineno + 1) * SCR_WIDTH) * 2
				   - 4);
		*pch++ = lineno / 10 + '0';
		*pch++ = RED_CHAR;
		*pch++ = lineno % 10 + '0';
		*pch++ = RED_CHAR;
	}
#endif
*/
}

/*****************************************************************************
 *                                w_copy
 *****************************************************************************/
/**
 * Copy data in WORDS.
 *
 * Note that the addresses of dst and src are not pointers, but integers, 'coz
 * in most cases we pass integers into it as parameters.
 *
 * @param dst   Addr of destination.
 * @param src   Addr of source.
 * @param size  How many words will be copied.
 *****************************************************************************/
PRIVATE	void w_copy(unsigned int dst, const unsigned int src, int size)
{
	memcpy((void*)(K_PHY2LIN(V_MEM_BASE) + (dst << 1)),
		  (void*)(K_PHY2LIN(V_MEM_BASE) + (src << 1)),
		  size << 1);
}


PUBLIC void disp_color_str(const char* info, int color){

	if(kernel_initial == 1){
		// disp_pos = _disp_color_str(info, color, disp_pos);
		disp_pos = vga_write_colored_str(info, color, disp_pos);
	}else{
		acquire(&video_mem_lock);
		CONSOLE* con = &console_table[current_console];
		con->cursor = vga_write_colored_str(info, color, con->cursor << 1) >> 1;
		// con->cursor = _disp_color_str(info, color, con->cursor << 1) >> 1;
		flush(con);
	#ifdef OPT_DISP_SERIAL
	#include <kernel/uart.h>
	for(char *p = info; p && *p; p++) {
		write_serial(*p);
		if(*p == '\n'){
			char tick_str[8];
			itoa(ticks, tick_str, 10);
			write_serial('[');
			for(char *s=tick_str; *s; s++){
				write_serial(*s);
			}
			write_serial(']');
		}
	}
	#endif
		release(&video_mem_lock);
	}
}

// PUBLIC void disp_color_str(const char* info, int color){

// 	if(kernel_initial == 1){
// 		disp_pos = _disp_color_str(info, color, disp_pos);
// 	}else{
// 		acquire(&video_mem_lock);
// 		CONSOLE* con = &console_table[current_console];
// 		con->cursor = _disp_color_str(info, color, con->cursor << 1) >> 1;
// 		flush(con);
// 	#ifdef OPT_DISP_SERIAL
// 	#include <kernel/uart.h>
// 	for(char *p = info; p && *p; p++) {
// 		write_serial(*p);
// 		if(*p == '\n'){
// 			char tick_str[8];
// 			itoa(ticks, tick_str, 10);
// 			write_serial('[');
// 			for(char *s=tick_str; *s; s++){
// 				write_serial(*s);
// 			}
// 			write_serial(']');
// 		}
// 	}
// 	#endif
// 		release(&video_mem_lock);
// 	}
// }

PUBLIC void disp_str(const char* info){
	disp_color_str(info, DEFAULT_CHAR_COLOR);
}

/*======================================================================*
                               disp_int
 *======================================================================*/
PUBLIC void disp_int(int input)
{
	char output[16];
	itoa(input, output, 16);
	disp_str(output);
}
