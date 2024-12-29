#include <stdio.h>
#include <string.h>
#include <ushm.h>

#define key 123456

int main(int arg, char *argv[]) {
    int q = fork();

    if (q == 0) {
        int shmidA = -1;
        printf("\nchirldA: ");
        shmidA = shmget(key, 80, SHM_IPC_CREAT);
        int *p = NULL;

        p = shmat(shmidA, NULL, 0);

        int i = 0;
        int q[15] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7};
        memcpy(p, q, sizeof(q));
        for (i = 0; i < 14; i++) { printf("%d ", *(p + i)); }

        shmdt(p);
    } else {
        int t;

        t = fork();
        if (t == 0) {
            int shmidB = -1;
            sleep(500);
            printf("\nchirldB: ");
            shmidB = shmget(key, 80, SHM_IPC_CREAT);
            int *pp = NULL;
            pp = shmat(shmidB, NULL, 0);

            /**********buf*******/
            int i = 0;
            for (i = 0; i < 14; i++) { printf("%d ", (*(pp + i))); }

            *pp = 5;
            // int reverse[15]={7,9,8,5,3,9,6,2,9,5,1,4,1,3};
            // memcpy(pp,reverse,sizeof(reverse));

            shmdt(pp);

            shmctl(shmidB, DELETE, NULL);
        }
    }

    exit(0);
    return 0;
}
