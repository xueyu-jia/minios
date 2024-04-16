#include "stdio.h"
#include "syscall.h"
// test vfs path 20240416 jiangfeng
// must run in root(/) dir


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

int main(int argc, char **argv) {

}