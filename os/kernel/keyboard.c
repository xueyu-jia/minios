#include <kernel/console.h>
#include <kernel/const.h>
#include <kernel/keyboard.h>
#include <kernel/keymap.h>
#include <kernel/protect.h>
#include <kernel/proto.h>
#include <kernel/tty.h>

PRIVATE KB_INPUT kb_in;
PRIVATE MOUSE_INPUT mouse_in;
PRIVATE int mouse_init;

/* Keymap for US MF-2 keyboard. */

u32 keymap[NR_SCAN_CODES * MAP_COLS] = {

    /* scan-code                    !Shift          Shift           E0 XX   */
    /* ==================================================================== */
    /* 0x00 - none          */ 0,
    0,
    0,
    /* 0x01 - ESC           */ ESC,
    ESC,
    0,
    /* 0x02 - '1'           */ '1',
    '!',
    0,
    /* 0x03 - '2'           */ '2',
    '@',
    0,
    /* 0x04 - '3'           */ '3',
    '#',
    0,
    /* 0x05 - '4'           */ '4',
    '$',
    0,
    /* 0x06 - '5'           */ '5',
    '%',
    0,
    /* 0x07 - '6'           */ '6',
    '^',
    0,
    /* 0x08 - '7'           */ '7',
    '&',
    0,
    /* 0x09 - '8'           */ '8',
    '*',
    0,
    /* 0x0A - '9'           */ '9',
    '(',
    0,
    /* 0x0B - '0'           */ '0',
    ')',
    0,
    /* 0x0C - '-'           */ '-',
    '_',
    0,
    /* 0x0D - '='           */ '=',
    '+',
    0,
    /* 0x0E - BS            */ BACKSPACE,
    BACKSPACE,
    0,
    /* 0x0F - TAB           */ TAB,
    TAB,
    0,
    /* 0x10 - 'q'           */ 'q',
    'Q',
    0,
    /* 0x11 - 'w'           */ 'w',
    'W',
    0,
    /* 0x12 - 'e'           */ 'e',
    'E',
    0,
    /* 0x13 - 'r'           */ 'r',
    'R',
    0,
    /* 0x14 - 't'           */ 't',
    'T',
    0,
    /* 0x15 - 'y'           */ 'y',
    'Y',
    0,
    /* 0x16 - 'u'           */ 'u',
    'U',
    0,
    /* 0x17 - 'i'           */ 'i',
    'I',
    0,
    /* 0x18 - 'o'           */ 'o',
    'O',
    0,
    /* 0x19 - 'p'           */ 'p',
    'P',
    0,
    /* 0x1A - '['           */ '[',
    '{',
    0,
    /* 0x1B - ']'           */ ']',
    '}',
    0,
    /* 0x1C - CR/LF         */ ENTER,
    ENTER,
    PAD_ENTER,
    /* 0x1D - l. Ctrl       */ CTRL_L,
    CTRL_L,
    CTRL_R,
    /* 0x1E - 'a'           */ 'a',
    'A',
    0,
    /* 0x1F - 's'           */ 's',
    'S',
    0,
    /* 0x20 - 'd'           */ 'd',
    'D',
    0,
    /* 0x21 - 'f'           */ 'f',
    'F',
    0,
    /* 0x22 - 'g'           */ 'g',
    'G',
    0,
    /* 0x23 - 'h'           */ 'h',
    'H',
    0,
    /* 0x24 - 'j'           */ 'j',
    'J',
    0,
    /* 0x25 - 'k'           */ 'k',
    'K',
    0,
    /* 0x26 - 'l'           */ 'l',
    'L',
    0,
    /* 0x27 - ';'           */ ';',
    ':',
    0,
    /* 0x28 - '\''          */ '\'',
    '"',
    0,
    /* 0x29 - '`'           */ '`',
    '~',
    0,
    /* 0x2A - l. SHIFT      */ SHIFT_L,
    SHIFT_L,
    0,
    /* 0x2B - '\'           */ '\\',
    '|',
    0,
    /* 0x2C - 'z'           */ 'z',
    'Z',
    0,
    /* 0x2D - 'x'           */ 'x',
    'X',
    0,
    /* 0x2E - 'c'           */ 'c',
    'C',
    0,
    /* 0x2F - 'v'           */ 'v',
    'V',
    0,
    /* 0x30 - 'b'           */ 'b',
    'B',
    0,
    /* 0x31 - 'n'           */ 'n',
    'N',
    0,
    /* 0x32 - 'm'           */ 'm',
    'M',
    0,
    /* 0x33 - ','           */ ',',
    '<',
    0,
    /* 0x34 - '.'           */ '.',
    '>',
    0,
    /* 0x35 - '/'           */ '/',
    '?',
    PAD_SLASH,
    /* 0x36 - r. SHIFT      */ SHIFT_R,
    SHIFT_R,
    0,
    /* 0x37 - '*'           */ '*',
    '*',
    0,
    /* 0x38 - ALT           */ ALT_L,
    ALT_L,
    ALT_R,
    /* 0x39 - ' '           */ ' ',
    ' ',
    0,
    /* 0x3A - CapsLock      */ CAPS_LOCK,
    CAPS_LOCK,
    0,
    /* 0x3B - F1            */ F1,
    F1,
    0,
    /* 0x3C - F2            */ F2,
    F2,
    0,
    /* 0x3D - F3            */ F3,
    F3,
    0,
    /* 0x3E - F4            */ F4,
    F4,
    0,
    /* 0x3F - F5            */ F5,
    F5,
    0,
    /* 0x40 - F6            */ F6,
    F6,
    0,
    /* 0x41 - F7            */ F7,
    F7,
    0,
    /* 0x42 - F8            */ F8,
    F8,
    0,
    /* 0x43 - F9            */ F9,
    F9,
    0,
    /* 0x44 - F10           */ F10,
    F10,
    0,
    /* 0x45 - NumLock       */ NUM_LOCK,
    NUM_LOCK,
    0,
    /* 0x46 - ScrLock       */ SCROLL_LOCK,
    SCROLL_LOCK,
    0,
    /* 0x47 - Home          */ PAD_HOME,
    '7',
    HOME,
    /* 0x48 - CurUp         */ PAD_UP,
    '8',
    UP,
    /* 0x49 - PgUp          */ PAD_PAGEUP,
    '9',
    PAGEUP,
    /* 0x4A - '-'           */ PAD_MINUS,
    '-',
    0,
    /* 0x4B - Left          */ PAD_LEFT,
    '4',
    LEFT,
    /* 0x4C - MID           */ PAD_MID,
    '5',
    0,
    /* 0x4D - Right         */ PAD_RIGHT,
    '6',
    RIGHT,
    /* 0x4E - '+'           */ PAD_PLUS,
    '+',
    0,
    /* 0x4F - End           */ PAD_END,
    '1',
    END,
    /* 0x50 - Down          */ PAD_DOWN,
    '2',
    DOWN,
    /* 0x51 - PgDown        */ PAD_PAGEDOWN,
    '3',
    PAGEDOWN,
    /* 0x52 - Insert        */ PAD_INS,
    '0',
    INSERT,
    /* 0x53 - Delete        */ PAD_DOT,
    '.',
    DELETE,
    /* 0x54 - Enter         */ 0,
    0,
    0,
    /* 0x55 - ???           */ 0,
    0,
    0,
    /* 0x56 - ???           */ 0,
    0,
    0,
    /* 0x57 - F11           */ F11,
    F11,
    0,
    /* 0x58 - F12           */ F12,
    F12,
    0,
    /* 0x59 - ???           */ 0,
    0,
    0,
    /* 0x5A - ???           */ 0,
    0,
    0,
    /* 0x5B - ???           */ 0,
    0,
    GUI_L,
    /* 0x5C - ???           */ 0,
    0,
    GUI_R,
    /* 0x5D - ???           */ 0,
    0,
    APPS,
    /* 0x5E - ???           */ 0,
    0,
    0,
    /* 0x5F - ???           */ 0,
    0,
    0,
    /* 0x60 - ???           */ 0,
    0,
    0,
    /* 0x61 - ???           */ 0,
    0,
    0,
    /* 0x62 - ???           */ 0,
    0,
    0,
    /* 0x63 - ???           */ 0,
    0,
    0,
    /* 0x64 - ???           */ 0,
    0,
    0,
    /* 0x65 - ???           */ 0,
    0,
    0,
    /* 0x66 - ???           */ 0,
    0,
    0,
    /* 0x67 - ???           */ 0,
    0,
    0,
    /* 0x68 - ???           */ 0,
    0,
    0,
    /* 0x69 - ???           */ 0,
    0,
    0,
    /* 0x6A - ???           */ 0,
    0,
    0,
    /* 0x6B - ???           */ 0,
    0,
    0,
    /* 0x6C - ???           */ 0,
    0,
    0,
    /* 0x6D - ???           */ 0,
    0,
    0,
    /* 0x6E - ???           */ 0,
    0,
    0,
    /* 0x6F - ???           */ 0,
    0,
    0,
    /* 0x70 - ???           */ 0,
    0,
    0,
    /* 0x71 - ???           */ 0,
    0,
    0,
    /* 0x72 - ???           */ 0,
    0,
    0,
    /* 0x73 - ???           */ 0,
    0,
    0,
    /* 0x74 - ???           */ 0,
    0,
    0,
    /* 0x75 - ???           */ 0,
    0,
    0,
    /* 0x76 - ???           */ 0,
    0,
    0,
    /* 0x77 - ???           */ 0,
    0,
    0,
    /* 0x78 - ???           */ 0,
    0,
    0,
    /* 0x78 - ???           */ 0,
    0,
    0,
    /* 0x7A - ???           */ 0,
    0,
    0,
    /* 0x7B - ???           */ 0,
    0,
    0,
    /* 0x7C - ???           */ 0,
    0,
    0,
    /* 0x7D - ???           */ 0,
    0,
    0,
    /* 0x7E - ???           */ 0,
    0,
    0,
    /* 0x7F - ???           */ 0,
    0,
    0};

PRIVATE int code_with_E0;
PRIVATE int shift_l;     /* l shift state	*/
PRIVATE int shift_r;     /* r shift state	*/
PRIVATE int alt_l;       /* l alt state		*/
PRIVATE int alt_r;       /* r left state		*/
PRIVATE int ctrl_l;      /* l ctrl state		*/
PRIVATE int ctrl_r;      /* l ctrl state		*/
PRIVATE int caps_lock;   /* Caps Lock		*/
PRIVATE int num_lock;    /* Num Lock		*/
PRIVATE int scroll_lock; /* Scroll Lock		*/
PRIVATE int column;

PRIVATE u8 get_byte_from_kb_buf();
PRIVATE void set_leds();
PRIVATE void set_mouse_leds();
PRIVATE void kb_wait();
PRIVATE void kb_ack();

void kb_handler(int irq) {
  UNUSED(irq);
  // disp_str("kbkbkb");
  u8 scan_code = inb(0x60);
  if (kb_in.count < KB_inbS) {
    *(kb_in.p_head) = scan_code;
    kb_in.p_head++;
    if (kb_in.p_head == kb_in.buf + KB_inbS) {
      kb_in.p_head = kb_in.buf;
    }
    kb_in.count++;
    wake_the_tty();
  }
};

#define TTY_FIRST (tty_table)
#define TTY_END (tty_table + NR_CONSOLES)

void mouse_handler(int irq) {
  UNUSED(irq);
  u8 scan_code = inb(0x60);
  if (!mouse_init) {
    mouse_init = 1;
    return;
  }
  mouse_in.buf[mouse_in.count] = scan_code;
  mouse_in.count++;
  if (mouse_in.count == 3) {
    TTY* p_tty;
    for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
      if (p_tty->console == &console_table[current_console]) {
        p_tty->mouse_left_button = mouse_in.buf[0] & 0x01;

        u8 mid_button = mouse_in.buf[0] & 0x4;
        if (mid_button == 0x4) {
          p_tty->mouse_mid_button = 1;
        } else {
          p_tty->mouse_mid_button = 0;
        }

        if (p_tty->mouse_left_button) {
          u8 dir_Y = mouse_in.buf[0] & 0x20;
          u8 dir_X = mouse_in.buf[0] & 0x10;
          if (dir_Y == 0x20) {  // down
            p_tty->mouse_Y -= 1;
          } else {  // up
            p_tty->mouse_Y += 1;
          }

          if (dir_X == 0x10) {  // left
            p_tty->mouse_X -= 1;
          } else {  // right
            p_tty->mouse_X += 1;
          }
        }
      }
    }
    wake_the_tty();
    mouse_in.count = 0;
  }
}

PRIVATE void init_mouse() {
  mouse_in.count = 0;

  put_irq_handler(MOUSE_IRQ, mouse_handler);
  enable_irq(MOUSE_IRQ);
}

PUBLIC void init_kb() {
  kb_in.count = 0;
  kb_in.p_head = kb_in.p_tail = kb_in.buf;

  shift_l = shift_r = 0;
  alt_l = alt_r = 0;
  ctrl_l = ctrl_r = 0;

  caps_lock = 0;
  num_lock = 1;
  scroll_lock = 0;

  column = 0;

  set_leds();
  put_irq_handler(KEYBOARD_IRQ, kb_handler);
  enable_irq(KEYBOARD_IRQ);

  init_mouse();
  set_mouse_leds();
}

PUBLIC void keyboard_read(TTY* p_tty) {
  u8 scan_code;

  /**
   * 1 : make
   * 0 : break
   */
  int make;

  /**
   * We use a integer to record a key press.
   * For instance, if the key HOME is pressed, key will be evaluated to
   * `HOME' defined in keyboard.h.
   */
  u32 key = 0;

  /**
   * This var points to a row in keymap[]. I don't use two-dimension
   * array because I don't like it.
   */
  u32* keyrow;

  while (kb_in.count > 0) {
    code_with_E0 = 0;
    scan_code = get_byte_from_kb_buf();

    /* parse the scan code below */
    if (scan_code == 0xE1) {
      int i;
      u8 pausebreak_scan_code[] = {0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5};
      int is_pausebreak = 1;
      for (i = 1; i < 6; i++) {
        if (get_byte_from_kb_buf() != pausebreak_scan_code[i]) {
          is_pausebreak = 0;
          break;
        }
      }
      if (is_pausebreak) {
        key = PAUSEBREAK;
      }
    } else if (scan_code == 0xE0) {
      code_with_E0 = 1;
      scan_code = get_byte_from_kb_buf();

      /* PrintScreen is pressed */
      if (scan_code == 0x2A) {
        code_with_E0 = 0;
        if ((scan_code = get_byte_from_kb_buf()) == 0xE0) {
          code_with_E0 = 1;
          if ((scan_code = get_byte_from_kb_buf()) == 0x37) {
            key = PRINTSCREEN;
            make = 1;
          }
        }
      }
      /* PrintScreen is released */
      else if (scan_code == 0xB7) {
        code_with_E0 = 0;
        if ((scan_code = get_byte_from_kb_buf()) == 0xE0) {
          code_with_E0 = 1;
          if ((scan_code = get_byte_from_kb_buf()) == 0xAA) {
            key = PRINTSCREEN;
            make = 0;
          }
        }
      }
    }

    if ((key != PAUSEBREAK) && (key != PRINTSCREEN)) {
      int caps;

      /* make or break */
      make = (scan_code & FLAG_BREAK ? 0 : 1);

      keyrow = &keymap[(scan_code & 0x7F) * MAP_COLS];

      column = 0;

      caps = shift_l || shift_r;
      if (caps_lock && keyrow[0] >= 'a' && keyrow[0] <= 'z') caps = !caps;

      if (caps) column = 1;

      if (code_with_E0) column = 2;

      key = keyrow[column];

      switch (key) {
        case SHIFT_L:
          shift_l = make;
          break;
        case SHIFT_R:
          shift_r = make;
          break;
        case CTRL_L:
          ctrl_l = make;
          break;
        case CTRL_R:
          ctrl_r = make;
          break;
        case ALT_L:
          alt_l = make;
          break;
        case ALT_R:
          alt_l = make;
          break;
        case CAPS_LOCK:
          if (make) {
            caps_lock = !caps_lock;
            set_leds();
          }
          break;
        case NUM_LOCK:
          if (make) {
            num_lock = !num_lock;
            set_leds();
          }
          break;
        case SCROLL_LOCK:
          if (make) {
            scroll_lock = !scroll_lock;
            set_leds();
          }
          break;
        default:
          break;
      }
    }

    if (make) { /* Break Code is ignored */
      int pad = 0;

      /* deal with the numpad first */
      if ((key >= PAD_SLASH) && (key <= PAD_9)) {
        pad = 1;
        switch (key) { /* '/', '*', '-', '+',
                        * and 'Enter' in num pad
                        */
          case PAD_SLASH:
            key = '/';
            break;
          case PAD_STAR:
            key = '*';
            break;
          case PAD_MINUS:
            key = '-';
            break;
          case PAD_PLUS:
            key = '+';
            break;
          case PAD_ENTER:
            key = ENTER;
            break;
          default:
            /* the value of these keys
             * depends on the Numlock
             */
            if (num_lock) { /* '0' ~ '9' and '.' in num pad */
              if (key >= PAD_0 && key <= PAD_9)
                key = key - PAD_0 + '0';
              else if (key == PAD_DOT)
                key = '.';
            } else {
              switch (key) {
                case PAD_HOME:
                  key = HOME;
                  break;
                case PAD_END:
                  key = END;
                  break;
                case PAD_PAGEUP:
                  key = PAGEUP;
                  break;
                case PAD_PAGEDOWN:
                  key = PAGEDOWN;
                  break;
                case PAD_INS:
                  key = INSERT;
                  break;
                case PAD_UP:
                  key = UP;
                  break;
                case PAD_DOWN:
                  key = DOWN;
                  break;
                case PAD_LEFT:
                  key = LEFT;
                  break;
                case PAD_RIGHT:
                  key = RIGHT;
                  break;
                case PAD_DOT:
                  key = DELETE;
                  break;
                default:
                  break;
              }
            }
            break;
        }
      }
      key |= shift_l ? FLAG_SHIFT_L : 0;
      key |= shift_r ? FLAG_SHIFT_R : 0;
      key |= ctrl_l ? FLAG_CTRL_L : 0;
      key |= ctrl_r ? FLAG_CTRL_R : 0;
      key |= alt_l ? FLAG_ALT_L : 0;
      key |= alt_r ? FLAG_ALT_R : 0;
      key |= pad ? FLAG_PAD : 0;

      in_process(p_tty, key);
    }
  }
}

/*****************************************************************************
 *                                get_byte_from_kb_buf
 *****************************************************************************/
/**
 * Read a byte from the keyboard buffer.
 *
 * @return The byte read.
 *****************************************************************************/
PRIVATE u8 get_byte_from_kb_buf() {
  u8 scan_code;

  while (kb_in.count <= 0) {
  } /* wait for a byte to arrive */

  disable_int(); /* for synchronization */
  scan_code = *(kb_in.p_tail);
  kb_in.p_tail++;
  if (kb_in.p_tail == kb_in.buf + KB_inbS) {
    kb_in.p_tail = kb_in.buf;
  }
  kb_in.count--;
  enable_int(); /* for synchronization */

  return scan_code;
}

/*****************************************************************************
 *                                kb_wait
 *****************************************************************************/
/**
 * Wait until the input buffer of 8042 is empty.
 *
 *****************************************************************************/
PRIVATE void kb_wait() /* 等待 8042 的输入缓冲区空 */
{
  u8 kb_stat;

  do {
    kb_stat = inb(KB_CMD);

  } while (kb_stat & 0x02);
}

/*****************************************************************************
 *                                kb_ack
 *****************************************************************************/
/**
 * Read from the keyboard controller until a KB_ACK is received.
 *
 *****************************************************************************/
MAYBE_UNUSED PRIVATE void kb_ack() {
  u8 kb_read;

  do {
    kb_read = inb(KB_DATA);
  } while (kb_read != KB_ACK);
}

/*****************************************************************************
 *                                set_leds
 *****************************************************************************/
/**
 * Set the leds according to: caps_lock, num_lock & scroll_lock.
 *
 *****************************************************************************/
PRIVATE void set_leds() {
  // u8 leds = (caps_lock << 2) | (num_lock << 1) | scroll_lock;

  // kb_wait();
  // outb(KB_DATA, LED_CODE);
  // kb_ack();

  // kb_wait();
  // outb(KB_DATA, leds);
  // kb_ack();
  kb_wait();
  outb(KB_CMD, KEYCMD_WRITE_MODE);
  kb_wait();
  outb(KB_DATA, KBC_MODE);
}

PRIVATE void set_mouse_leds() {
  kb_wait();
  outb(KB_CMD, KBCMD_EN_MOUSE_INTFACE);
  kb_wait();
  outb(KB_CMD, KEYCMD_SENDTO_MOUSE);
  kb_wait();
  outb(KB_DATA, MOUSECMD_ENABLE);
  kb_wait();
  outb(KB_CMD, KEYCMD_WRITE_MODE);
  kb_wait();
  outb(KB_DATA, KBC_MODE);
}
