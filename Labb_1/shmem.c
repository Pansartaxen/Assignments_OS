#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <stdlib.h>
#define SHMSIZE 128
#define SHM_R 0400
#define SHM_W 0200
int main(int argc, char **argv)
{
    struct shm_struct {
    int buffer[10];
    int used;
    };

    volatile struct shm_struct *shmp = NULL;
    char *addr = NULL;
    pid_t pid = -1;
    int var1 = 0, var2 = 0, shmid = -1;
    struct shmid_ds *shm_buf;

    /*allocate a chunk of shared memory */
    shmid = shmget(IPC_PRIVATE, SHMSIZE, IPC_CREAT | SHM_R | SHM_W);
    shmp = (struct shm_struct *) shmat(shmid, addr, 0);
    int index = 0; //Index for the array
    pid = fork();

    if (pid != 0) {
        /*here’s the parent, acting as producer */
        while (var1 < 100) {
            /*write to shmem */
            var1++;
            while (shmp->used == 10); /*busy wait until the buffer isn't full */
            printf("Sending %d to index %d\n", var1, index%10); fflush(stdout);
            shmp->buffer[index%10] = var1; //Write to the buffer
            shmp->used++; //Increase the count of numbers int the buffer
            int wait = ((rand()%4)/10)+0.1; //Computes the wait time
            index++; //Increments the index
            sleep(wait); //Sleep the randomly generated time
        }
        shmdt(addr);
        shmctl(shmid, IPC_RMID, shm_buf);
    } 
    else {
        /*here’s the child, acting as consumer */
        while (var2 < 100) {
            /*read from shmem */
            while (shmp->used == 0); /*busy wait until the buffer isn't empty */
            var2 = shmp->buffer[index%10];
            shmp->used--; //Decrease the count of numbers int the buffer
            printf("Received %d from index %d\n", var2, index%10); fflush(stdout);
            int wait = ((rand()%18)/10)+0.2;
            sleep(wait);
            index++;
        }
        shmdt(addr);
        shmctl(shmid, IPC_RMID, shm_buf);
    }
    printf("\n");
    return 0;
}