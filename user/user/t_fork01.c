/*
    added by dzq 2023.3.29
    
    [Description]:

    

*/


#include "../include/stdio.h"

#define KIDEXIT 42  //这个数字目前无特殊含义

void main(int arg, char *argv[]){
    printf("before fork: pid == %d\n", get_pid());
    int pid, term_pid;
    pid = fork();
    if(pid == 0){
        printf("childpid:%d\n", get_pid());
        exit(6);
    }
    else {
        int x;
        int s = -1;
        x = wait_(&s);
        printf("wait() send back:%d", x);
        printf("parentpid:%d\n", get_pid());
        printf("exit code:%d", s);
        exit(8);
    }
    /*
    term_pid = wait_();
    if(term_pid == pid){
        
    }
*/
    
    exit(0);

}


