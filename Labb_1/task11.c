/*
• Let the professors think a random time between one and five seconds.
• Take the left chopstick.
• When they have gotten the left chopstick, let them think in two to eight seconds.
• Take the right chopstick.
• Let the professor eat for a random time between five-ten seconds, and then put down both chopsticks.
• Write information to the terminal when the professors go from one state to another, e.g., "Tanenbaum: thinking ->
got left chopstick". Also indicate when a professor tries to take a chopstick.
There are some restrictions for your implementation.

• Each fork should be modeled as a shared resource, which exclusive usage.
• You are not allowed to look at the state of your neighbors to see if they are eating or not, i.e., the solution in Sec-
tion 2.5.1 in the course book [1] is not allowed.
• You are only allowed to see if the forks to the left and to the right of you are available.
• You are not allowed to lock the whole table when you examine the availability of the forks
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>


pthread_mutex_t chop_locks[5];

struct threadArgs
{
    /*Struct for sending the ID of every professor as argument
      to prof function*/
    unsigned int id;
};

int take_left_chop(void *id)
{
    //Enters if clause if the chop stick (lock) is availeble
    if (pthread_mutex_trylock(&chop_locks[*(int*)id]) == 0)
    {
        printf("Prof #%d is taking the left chopstick\n", *(int *)id);
        return 1;
    }
    return 0;
}

int take_right_chop(void *id)
{
    int index = *(int *)id;
    index = (index - 1) % 5; // To get the index of the right chopstick
    //Enters if clause if the chop stick (lock) is availeble
    if (pthread_mutex_trylock(&chop_locks[index]) == 0)
    {
        printf("Prof #%d is taking the right chopstick\n", *(int *)id);
        return 1;
    }
    return 0;
}

void release_chops(void *id)
{
    printf("Prof #%d is releasing its chop sticks\n", *(int *)id);
    /*Prof is done eating and releases both chop sticks*/
    int index = *(int *)id;
    pthread_mutex_unlock(&chop_locks[index]);
    //Modulo 5 to stay in the range of the array
    index = (index - 1) % 5;
    pthread_mutex_unlock(&chop_locks[index]);
}

void *prof(void *id)
{
    //Volatile to prevent compiler optimization
    volatile int cur_chops = 0;
    volatile int loops = 0;
    double wait = ((rand() % 4) / 10.f) + 0.1;
    printf("Prof #%d is thinking for %f seconds\n", *(int *)id, wait);
    usleep(wait * 1000000);
    while (cur_chops != 2)
    {
        if (take_left_chop(id) == 1 && cur_chops == 0)
        {
            cur_chops++;
            loops++;
            wait = ((rand() % 6) / 10.f) + 0.2;
            printf("Prof #%d is thinking for %f seconds\n", *(int *)id, wait);
            usleep(wait * 1000000);
        }
        else if (take_right_chop(id) == 1 && cur_chops == 1)
        {
            cur_chops++;
            wait = ((rand() % 5) / 10.f) + 0.5;
            printf("Prof #%d is eating for %f seconds\n", *(int *)id, wait);
            usleep(wait * 1000000);
        }
        else if (loops < 5)
        {
            printf("Prof #%d is giving up its chop sticks\n", *(int *)id);
            int index = *(int *)id;
            pthread_mutex_unlock(&chop_locks[index]);
            cur_chops = 0;
            loops = 0;
        }
    }
    printf("Prof #%d is eating\n", *(int *)id);
    usleep(wait * 1000000);
    release_chops(id);
    return NULL;
}

int main(void)
{
    srand(time(0));
    pthread_t *children;
    children = malloc(5 * sizeof(pthread_t));
    struct threadArgs *args;
    args = malloc(5 * sizeof(struct threadArgs));

    //Initialize the locks
    for (int i = 0; i < 5; i++)
    {
        pthread_mutex_init(&chop_locks[i], NULL);
    }

    //Create 5 threads
    for (int id = 0; id < 5; id++)
    {
        args[id].id = id;
        pthread_create(&(children[id]), NULL, prof, (void *)&args[id]);
    }

    //Waits for all threads to finish
    for (int id = 0; id < 5; id++)
    {
        pthread_join(children[id], NULL);
    }
    //Free the allocated memmory to prevent emory leaks
    free(children);
    free(args);
    //Destroy all the locks
    for (int i = 0; i < 5; i++)
    {
        pthread_mutex_destroy(&chop_locks[i]);
    }
    printf("Every professor has eaten its noodles :)\n");
    return 0;
}