//ported by sundong 2023.3.26
#include "loaderprint.h"
#include "string.h"
int video_row = 0;
int video_col = 0;
// 清除屏幕
void clear_screen() {
  char *video_memory = (char *)(VIDEO_MEM_START);
  for (int i = 0; i < TERMINAL_ROW; i++)
    for (int j = 0; j < TERMINAL_COLUMN; j++) {
      // 设置颜色,在清理屏幕的时候使用
      *video_memory = 0x0;
      video_memory = video_memory + 1;
      *video_memory = 0xf;
      video_memory = video_memory + 1;
    }
    video_col = 0;
    video_row = 0;
}

void lprintf(char *s, ...)  // 传入输出的行与列,然后输入需要输出的字符串,根据字符串输出后面的信息
{
  if (video_row >= TERMINAL_ROW) {
    video_row = video_row % TERMINAL_ROW;
  }
  if (video_col > TERMINAL_COLUMN) {
    video_col = video_col % TERMINAL_COLUMN;
  }
  char *video_memory = (char *)(VIDEO_MEM_START + video_row * TERMINAL_COLUMN * 2 + video_col * 2);
  va_list ap;
  va_start(ap, s);
  while (*s != 0) {
    if (*s == '%')  // 遇到百分号
    {
      s++;
      if (*s == 0)  // 百分号后面没有东西
      {
        *video_memory = '%';
        video_memory = video_memory + 2;
        video_col++;
        break;
      } else if (*s == 'd')  // 打印u32,十六进制，打印八位
      {
        u32 content;
        content = va_arg(ap, u32);
        int k = 0;
        for (u32 s2 = content; k < 8; k++) {
          u32 s1 = s2 & 0xf0000000;
          s2 = s2 << 4;
          u8 s3 = s1 >> 28;
          if (s3 > '9' - '0') {
            s3 = s3 + 'A' - ':';
          }
          *video_memory = s3 + '0';
          video_memory = video_memory + 2;
        }
        video_col+=8;
      } else if (*s == 'c') {
        u8 content;
        content = va_arg(ap, int);
        // 写入需要的字符
        *video_memory = content;
        video_memory = video_memory + 2;
        video_col++;
      } else {
        *video_memory = '%';
        video_memory = video_memory + 2;
        *video_memory = *s;
        video_memory = video_memory + 2;
        video_col+=2;
      }
    } else if (*s == '\n') {
      video_row+=1;
      video_col=0;
      video_memory = (char *)(VIDEO_MEM_START + video_row * TERMINAL_COLUMN * 2+video_col * 2);
    } else {
      *video_memory = *s;
      video_memory = video_memory + 2;
      video_col++;
    }
    s++;
  }
  va_end(ap);
}
// 打印内存信息

void print_mem(struct ARDStruct *adr) {
  lprintf("%d %d %d %d %d\n", (*adr).dwBaseAddrLow, (*adr).dwBaseAddrHigh,
          (*adr).dwLengthLow, (*adr).dwLengthHigh, (*adr).dwType);
}
void print_elf(struct Proghdr *ph ) {
  lprintf("%d %d %d %d\n", ph->p_type, ph->p_va, ph->p_filesz,
          ph->p_offset);
 
}
