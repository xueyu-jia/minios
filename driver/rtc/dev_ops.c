#include <driver/rtc/dev_ops.h>
#include <driver/rtc/rtc.h>
#include <minios/time.h>
#include <minios/dev.h>
#include <fmt.h>
#include <string.h>

static int rtc_file_read(struct file_desc* file, unsigned int count, char* buf) {
    const int dev = file->fd_dentry->d_inode->i_b_cdev;
    if (DEV_MAJOR(dev) != DEV_CHAR_RTC) { return -1; }
    const int nr_rtc = DEV_MINOR(dev);
    //! TODO: support more rtc clock
    if (nr_rtc != 0) { return -1; }
    static size_t last_rd_off = 0;
    static size_t tm_len = 0;
    struct tm tm = {};
    char tm_buf[64] = {};
    if (last_rd_off == 0) {
        kern_get_time(&tm);
        nstrfmt(tm_buf, sizeof(tm_buf), "%4d-%02d-%02d %02d:%02d:%02d%+03d:00", tm.tm_year + 1900,
                tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, LOCAL_TIMEZONE);
        tm_len = strlen(tm_buf);
    }
    if (tm_len < last_rd_off + count) {
        memcpy(buf, tm_buf, tm_len);
        const int rd_cnt = tm_len - last_rd_off;
        last_rd_off = 0;
        return rd_cnt;
    }
    if (last_rd_off == tm_len) {
        last_rd_off = 0;
        return 0;
    }
    memcpy(buf, tm_buf + last_rd_off, count);
    last_rd_off += count;
    return count;
}

struct file_operations rtc_file_ops = {
    .read = rtc_file_read,
    .write = NULL,
};
