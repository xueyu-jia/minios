#include "usertest.h"

const char *test_name = "execve07";
int main(int argc, char *argv[], char *envp[])
{
    int i = 0;

    printf("hello_pid: %d\n", get_pid());

    for (i = 0; envp[i] != NULL; i++) {
        printf("%s\n", envp[i]);
    }

    int retval = strncmp("execve-envp-test", envp[0], strlen(envp[0]));
    if(retval == 0){
        printf("%s: passed\n", test_name);
        exit(TC_PASS);
    }else{
        printf("%s: envp[%d]: %s, expected: %s\n", test_name, 0, envp[0],
             "execve-envp-test");
        exit(-1);
    }
}
