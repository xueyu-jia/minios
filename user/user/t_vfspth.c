#include "stdio.h"
#include "syscall.h"
#include "string.h"
// test vfs path 20240416 jiangfeng
// must run in root(/) dir

#define EXPECT_ZERO(exp, testinfo)  if((exp) != 0){printf("%s failed\n", #testinfo);exit(-1);}
#define EXPECT_NONZERO(exp, testinfo)  if((exp) == 0){printf("%s failed\n", #testinfo);exit(-1);}
#define EXPECT_NONNEG(exp, testinfo)  if((exp) < 0){printf("%s failed\n", #testinfo);exit(-1);}
#define EXPECT_NEG(exp, testinfo)  if((exp) >= 0){printf("%s failed\n", #testinfo);exit(-1);}
void
rmdot(void)
{
  printf("rmdot test\n");
  if(mkdir("dots", I_RW) != 0){
    printf("mkdir dots failed\n");
    exit(-1);
  }
  if(chdir("dots") != 0){
    printf("chdir dots failed\n");
    exit(-1);
  }
  if(rmdir(".") == 0){
    printf("rm . worked!\n");
    exit(-1);
  }
  if(rmdir("..") == 0){
    printf("rm .. worked!\n");
    exit(-1);
  }
  if(chdir("/") != 0){
    printf("chdir / failed\n");
    exit(-1);
  }
  if(rmdir("dots/.") == 0){
    printf("unlink dots/. worked!\n");
    exit(-1);
  }
  if(rmdir("dots/..") == 0){
    printf("unlink dots/.. worked!\n");
    exit(-1);
  }
  if(rmdir("dots") != 0){
    printf("unlink dots failed!\n");
    exit(-1);
  }
  printf("rmdot ok\n");
}
char path[MAX_PATH];
char cwd[MAX_PATH];
char tmp[MAX_PATH];

int main(int argc, char **argv) {
    getcwd(cwd, MAX_PATH);
    EXPECT_ZERO(mkdir("test_path", I_RWX), make testdir)
    EXPECT_ZERO(chdir("test_path"), change into dir)
    int base_path_len = strlen(cwd);
    if(cwd[base_path_len-1]!='/'){
        strcat(cwd, "/");
    }
    strcat(cwd, "test_path");
    getcwd(tmp, MAX_PATH);
    EXPECT_ZERO(strcmp(cwd, tmp), check cwd after cd)
    EXPECT_ZERO(chdir("."), cd into .)
    getcwd(path, MAX_PATH);
    EXPECT_ZERO(strcmp(cwd, path), cd into . not change cwd)
    EXPECT_ZERO(chdir(".."), cd into ..)
    cwd[base_path_len] = 0;
    getcwd(tmp, MAX_PATH);
    EXPECT_ZERO(strcmp(cwd, tmp), cd into .. cwd back to origin)
    EXPECT_NEG(chdir("test_path_non_exist/.."), cd into non-exist-dir/..)
    EXPECT_ZERO(chdir("test_path/.."), cd into dir/..)
    getcwd(tmp, MAX_PATH);
    EXPECT_ZERO(strcmp(cwd, tmp), cd into dir/.. not change cwd)
    int fd;
    EXPECT_NONNEG(open("test_path/f", O_CREAT, 0), creat and open file)
    close(fd);
    EXPECT_NEG(rmdir("test_path"), rm non-empty dir)
    EXPECT_ZERO(unlink("./test_path/f"), unlink file)
    EXPECT_NEG(rmdir("test_path/."), rmdir invalid input .)
    EXPECT_NEG(rmdir("test_path/.."), rmdir invalid input ..)
    EXPECT_ZERO(chdir("./test_path/"), cd into ./dir/)
    EXPECT_ZERO(rmdir("../test_path"), rmdir ../cwd)
    getcwd(tmp, MAX_PATH);
    EXPECT_ZERO(strcmp(path, tmp), cwd after rmdir name unchange)
    EXPECT_ZERO(chdir(".."), back into parent)
    EXPECT_NEG(chdir("test_path"), cd into deleted dir)
    EXPECT_ZERO(chdir("/"), cd into root)
    EXPECT_ZERO(chdir(".."), cd .. in root)
    getcwd(tmp, MAX_PATH);
    EXPECT_ZERO(strcmp(tmp, "/"), root parent still root)
    printf("all path test ok");
    return 0;
}