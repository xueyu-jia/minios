#include <ctest/ctest.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <malloc.h>

TEST(Fork, Basic) {
    const int total_fork = 100;
    int total_pass = 0;
    for (int i = 0; i < total_fork; ++i) {
        const int pid = fork();
        if (pid < 0) { break; }
        if (pid == 0) { exit(i); }
        int status = 0;
        const int ch_pid = wait(&status);
        EXPECT_EQ(ch_pid, pid);
        EXPECT_EQ(status, i);
        if (!(ch_pid == pid && status == i)) { break; }
        ++total_pass;
    }
    EXPECT_EQ(total_pass, total_fork);
}

TEST(Fork, WaitAfterAll) {
    //! NOTE: we don't have that much pcbs reserved in kernel, and a fork failure is acceptable in
    //! this case

    typedef struct {
        bool forked;
        int pid;
        int exit_code;
    } fork_state_t;

    const int total_fork = 100;
    fork_state_t *fork_info = malloc(sizeof(fork_state_t) * total_fork);
    ASSERT_TRUE(fork_info != NULL);

    int total_forked = 0;
    for (int i = 0; i < total_fork; ++i) {
        const int pid = fork();
        if (pid < 0) {
            fork_info[i].forked = false;
            continue;
        } else if (pid == 0) {
            exit(i);
        }
        fork_info[i].forked = true;
        fork_info[i].pid = pid;
        fork_info[i].exit_code = i;
        ++total_forked;
    }

    int total_pass = 0;
    for (int i = 0; i < total_fork; ++i) {
        //! NOTE: here fork_info[i].forked doesn't refers to the i-st child, we only use it to
        //! decide how many times to perform wait
        if (!fork_info[i].forked) { continue; }
        int status = 0;
        const int ch_pid = wait(&status);
        for (int j = 0; j < total_fork; ++j) {
            if (ch_pid != fork_info[j].pid) { continue; }
            if (status == fork_info[j].exit_code) { ++total_pass; }
            break;
        }
    }
    EXPECT_EQ(total_pass, total_forked);

    free(fork_info);
}

TEST(Fork, DataCloned) {
    enum {
        PASS = 0,
        BAD_STACK = 0x1,
        BAD_HEAP = 0x2,
        UNKNOWN_ERROR = 0x4,
    };

    const char *magic = "TRICK OR TREAT";

    typedef struct {
        char _1;
        short _2;
        int _3;
        void *_4;
    } stack_ds_t;

    const stack_ds_t dat_stack = {
        ._1 = '1',
        ._2 = 2,
        ._3 = 3,
        ._4 = (void *)4,
    };

    const void *dat_heap = malloc_4k();
    ASSERT_TRUE(dat_heap != NULL);
    strcpy(dat_heap, magic);

    if (fork() == 0) {
        int retval = 0;
        if (!(dat_stack._1 == '1' && dat_stack._2 == 2 && dat_stack._3 == 3 &&
              dat_stack._4 == (void *)4)) {
            retval |= BAD_STACK;
        }
        if (fork() == 0) {
            //! fork again to avoid page fault from bad heap clone
            bool pass = strcmp(dat_heap, magic) == 0;
            free_4k(dat_heap); //<! force a free whatever error occurs
            exit(pass ? PASS : BAD_HEAP);
        }
        int status = 0;
        wait(&status);
        if (status < 0) { status = UNKNOWN_ERROR; }
        retval |= status;
        exit(retval);
    }

    int status = 0;
    wait(&status);
    EXPECT_EQ(status, PASS);

    free_4k(dat_heap);
}

TEST(Fork, Concurrency) {
    const int dur = 100;
    const int beg_tick = getticks();

    if (fork() == 0) {
        sleep(dur);
        exit(0);
    }

    sleep(dur);
    wait(NULL);

    const int end_tick = getticks();
    const int elapsed_tick = end_tick - beg_tick;
    EXPECT_LT(elapsed_tick, dur * 2);
}

TEST(Fork, SharedFileDescWithForkFirst) {
    /*!
     * 1. fa open file, then fork
     * 2. fa write
     * 3. ch write, fa wait ch
     * 4. fa write
     * 5. fa check file content
     */

    enum {
        PASS,
        BAD_CH_WR,
    };

    const char *filename = ".dummy.fork";
    const int fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, I_RW);
    ASSERT_TRUE(fd >= 0);

    const char *fa_text_1 = "abcd";
    const char *ch_text = "efgh";
    const char *fa_text_2 = "ijkl";

    char text_expected[256] = {};
    strcat(text_expected, fa_text_1);
    strcat(text_expected, ch_text);
    strcat(text_expected, fa_text_2);

    if (fork() == 0) {
        sleep(100);
        const int total_wr = write(fd, ch_text, strlen(ch_text));
        const bool pass = total_wr == strlen(ch_text);
        exit(pass ? PASS : BAD_CH_WR);
    }

    const int total_wr_1 = write(fd, fa_text_1, strlen(fa_text_1));
    EXPECT_EQ(total_wr_1, strlen(fa_text_1));

    int status = 0;
    wait(&status);
    EXPECT_EQ(status, PASS);

    const int total_wr_2 = write(fd, fa_text_2, strlen(fa_text_2));
    EXPECT_EQ(total_wr_2, strlen(fa_text_2));

    //! NOTE: we have implicit '\0' here
    char text_actual[256] = {};
    lseek(fd, 0, SEEK_SET);
    const int total_rd = read(fd, text_actual, sizeof(text_actual));
    EXPECT_EQ(total_rd, strlen(text_expected));
    EXPECT_STREQ(text_actual, text_expected);

    close(fd);
    unlink(filename);
}

TEST(Fork, SharedFileDescWithFileOpsFirst) {
    /*!
     * 1. fa open file
     * 2. fa write n-times, then fork n-ch
     * 3. each ch read once and check, fa wait ch
     * 4. fa check total read
     */

    enum {
        PASS,
        BAD_CH_RD,
    };

    const char *filename = ".dummy.fork";
    int fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, I_RW);
    ASSERT_TRUE(fd >= 0);

    const int total_fork = 30;
    const char char_wr = 'x';

    for (int i = 0; i < total_fork; ++i) { write(fd, &char_wr, 1); }
    close(fd);

    fd = open(filename, O_RDWR);
    ASSERT_TRUE(fd >= 0);

    int actual_forked = 0;
    for (int i = 0; i < total_fork; ++i) {
        const int pid = fork();
        if (pid < 0) { continue; }
        if (pid == 0) {
            char char_rd = 0;
            const int n = read(fd, &char_rd, 1);
            const bool pass = n == 1 && char_rd == char_wr;
            exit(pass ? PASS : BAD_CH_RD);
        }
        ++actual_forked;
    }

    int total_pass = 0;
    for (int i = 0; i < actual_forked; ++i) {
        int status = 0;
        wait(&status);
        if (status == PASS) { ++total_pass; }
    }

    EXPECT_EQ(total_pass, actual_forked);

    const int retval = lseek(fd, 0, SEEK_CUR);
    EXPECT_EQ(retval, actual_forked);

    char char_eof = 0;
    const int n = read(fd, &char_eof, 1);
    EXPECT_EQ(n, 0);

    close(fd);
    unlink(filename);
}

TEST(Fork, SharedFileDescCheckFileState) {
    /*!
     * 1. fa open file, then fork
     * 2. ch write to file, fa wait ch
     * 3. fa check file cursor
     */

    enum {
        PASS,
        BAD_CH_WR,
    };

    const char *filename = ".dummy.fork";
    int fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, I_RW);
    ASSERT_TRUE(fd >= 0);

    const int total_fork = 30;
    const char char_wr = 'x';

    int actual_forked = 0;
    for (int i = 0; i < total_fork; ++i) {
        const int pid = fork();
        if (pid < 0) { continue; }
        if (pid == 0) {
            const int n = write(fd, &char_wr, 1);
            const bool pass = n == 1;
            exit(pass ? PASS : BAD_CH_WR);
        }
        ++actual_forked;
    }

    int total_pass = 0;
    for (int i = 0; i < actual_forked; ++i) {
        int status = 0;
        wait(&status);
        if (status == PASS) { ++total_pass; }
    }

    EXPECT_EQ(total_pass, actual_forked);

    const int retval = lseek(fd, 0, SEEK_CUR);
    EXPECT_EQ(retval, actual_forked);

    char char_eof = 0;
    const int n = read(fd, &char_eof, 1);
    EXPECT_EQ(n, 0);

    close(fd);
    unlink(filename);
}

TEST(Fork, CloseFileDescInChild) {
    enum {
        PASS,
        BAD_CH_RD,
    };

    const char *filename = ".dummy.fork";
    int fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, I_RW);
    ASSERT_TRUE(fd >= 0);

    const char char_wr = 'x';
    write(fd, &char_wr, 1);
    close(fd);

    fd = open(filename, O_RDWR);
    ASSERT_TRUE(fd >= 0);

    if (fork() == 0) {
        close(fd);
        exit(0);
    }

    wait(NULL);

    if (fork() == 0) {
        char char_rd = 0;
        const int n = read(fd, &char_rd, 1);
        const bool pass = n == 1 && char_rd == char_wr;
        exit(pass ? PASS : BAD_CH_RD);
    } else {
        int status = 0;
        wait(&status);
        EXPECT_EQ(status, PASS);
    }

    char char_eof = 0;
    const int n = read(fd, &char_eof, 1);
    EXPECT_EQ(n, 0);

    close(fd);
    unlink(filename);
}
