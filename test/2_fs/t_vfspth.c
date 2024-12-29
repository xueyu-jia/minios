#include <stdio.h>
#include <string.h>
#include <syscall.h>

#define TEST_INFO(cond, testinfo)         \
    if (!(cond)) {                        \
        printf("%s failed\n", #testinfo); \
        exit(-1);                         \
    } else {                              \
        printf("%s ok\n", #testinfo);     \
    }

#define EXPECT_ZERO(exp, testinfo) TEST_INFO((exp) == 0, testinfo)
#define EXPECT_NONZERO(exp, testinfo) TEST_INFO((exp) != 0, testinfo)
#define EXPECT_NONNEG(exp, testinfo) TEST_INFO((exp) >= 0, testinfo)
#define EXPECT_NEG(exp, testinfo) TEST_INFO((exp) < 0, testinfo)

char path[MAX_PATH];
char cwd[MAX_PATH];
char tmp[MAX_PATH];

int main(int argc, char **argv) {
    getcwd(cwd, MAX_PATH);
    EXPECT_ZERO(mkdir("test_path", I_RWX), make testdir)
    EXPECT_ZERO(chdir("test_path"), change into dir)
    int base_path_len = strlen(cwd);
    if (cwd[base_path_len - 1] != '/') { strcat(cwd, "/"); }
    strcat(cwd, "test_path");
    getcwd(tmp, MAX_PATH);
    EXPECT_ZERO(strcmp(cwd, tmp), check cwd after cd)
    EXPECT_ZERO(chdir("."), cd into.)
    getcwd(path, MAX_PATH);
    EXPECT_ZERO(strcmp(cwd, path), cd into.not change cwd)
    EXPECT_ZERO(chdir(".."), cd into..)
    cwd[base_path_len] = 0;
    getcwd(tmp, MAX_PATH);
    EXPECT_ZERO(strcmp(cwd, tmp), cd into..cwd back to origin)
    EXPECT_NEG(chdir("test_path_non_exist/.."), cd into non - exist - dir /..)
    EXPECT_ZERO(chdir("test_path/.."), cd into dir /..)
    getcwd(tmp, MAX_PATH);
    EXPECT_ZERO(strcmp(cwd, tmp), cd into dir /..not change cwd)
    int fd;
    EXPECT_NONNEG(fd = open("test_path/f", O_CREAT, I_RW), creat and open file)
    close(fd);
    EXPECT_NEG(rmdir("test_path"), rm non - empty dir)
    EXPECT_ZERO(unlink("./test_path/f"), unlink file)
    EXPECT_NEG(rmdir("test_path/."), rmdir invalid input.)
    EXPECT_NEG(rmdir("test_path/.."), rmdir invalid input..)
    EXPECT_ZERO(chdir("./test_path/"), cd into./ dir /)
    EXPECT_ZERO(rmdir("../test_path"), rmdir../ cwd)
    getcwd(tmp, MAX_PATH);
    EXPECT_ZERO(strcmp(path, tmp), cwd after rmdir name unchange)
    EXPECT_ZERO(chdir(".."), back into parent)
    EXPECT_NEG(chdir("test_path"), cd into deleted dir)
    EXPECT_ZERO(chdir("/"), cd into root)
    EXPECT_ZERO(chdir(".."), cd..in root)
    getcwd(tmp, MAX_PATH);
    EXPECT_ZERO(strcmp(tmp, "/"), root parent still root)
    printf("all path test ok");
    return 0;
}
