#include <stdio.h>
#include <unistd.h>
//https://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/create.html

int main(int argc, char **argv){
    pid_t pid, pid2;
    unsigned i;
    unsigned niterations = 100;
    int status;
    pid = fork();
    if (pid == 0) {
        for (i = 0; i < niterations; ++i)
            printf("A = %d, ", i);
    }
    else{
        printf("Id for first child process: %i\n",pid);
        pid2 = fork();
        if (pid2 == 0) {
            for (i = 0; i < niterations; ++i)
                printf("C = %d, ", i);
        }
        else{
            printf("\nId for the second child process: %i\n", pid2);
            for (i = 0; i < niterations; ++i)
                printf("B = %d, ", i);
            printf("\n");
        }
    }
    printf("\n");

    return 0;
}