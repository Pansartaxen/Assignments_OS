#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h> /* For O_* constants */
#include <pthread.h>
#include <sys/wait.h>
#define SHMSIZE 128
#define SHM_R 0400
#define SHM_W 0200
const char *semName1 = "my_sema1";
const char *semName2 = "my_sema2";

//sem_t semaphore;

int main(int argc, char **argv)
{
    struct shm_struct {
    int buffer[10];
    int used;
    };

    volatile struct shm_struct *shmp = NULL;
    char *addr = NULL;
    pid_t pid = -1;
    sem_t *sem_id1 = sem_open(semName1, O_CREAT, O_RDWR, 10); 
    /*create semaphore with initial value of 10 (size of the buffer)
      so the producer will not write over a value that has not been read */
    sem_t *sem_id2 = sem_open(semName2, O_CREAT, O_RDWR, 0);
    /*create semaphore with initial value of 0
      (amount of produced but not read products)*/
    int i, status;
    int var1 = 0, var2 = 0, shmid = -1;
    struct shmid_ds *shm_buf;

    /*allocate a chunk of shared memory */
    shmid = shmget(IPC_PRIVATE, SHMSIZE, IPC_CREAT | SHM_R | SHM_W);
    shmp = (struct shm_struct *) shmat(shmid, addr, 0);
    int index = 0; //Index for the array
    pid = fork(); //creates two processes

    if (pid != 0) {
        /*here’s the parent, acting as producer*/
        while (var1 < 100) {
            /*write to shmem */
            var1++;
            sem_wait(sem_id1);
            shmp->buffer[index%10] = var1; //Write to the buffer
            shmp->used++; //Increase the count of numbers int the buffer
            sem_post(sem_id2);

            printf("Sending %d to index %d\n", var1, index%10); fflush(stdout);
            double wait = ((rand()%4)/10.f)+0.1; //Computes the wait time
            index++; //Increments the index
            usleep(1000000*wait); //Sleep the randomly generated time
        }
        shmdt(addr);
        shmctl(shmid, IPC_RMID, shm_buf);
        sem_close(sem_id1);
        sem_close(sem_id2);
        wait(&status);
        sem_unlink(semName1);
        sem_unlink(semName2);
    } 
    else {
        /*here’s the child, acting as consumer */
        while (var2 < 100) {
            /*read from shmem */

            sem_wait(sem_id2);
            var2 = shmp->buffer[index%10];
            shmp->used--; //Decrease the count of numbers int the buffer
            sem_post(sem_id1);

            printf("Received %d from index %d\n", var2, index%10); fflush(stdout);
            double wait = ((rand()%18)/10.f)+0.2; //Computes the wait time
            usleep(1000000*wait); //Sleep the randomly generated time
            index++;
        }
        shmdt(addr);
        shmctl(shmid, IPC_RMID, shm_buf);
        sem_close(sem_id1);
        sem_close(sem_id2);
    }
    printf("\n");
    return 0;
}