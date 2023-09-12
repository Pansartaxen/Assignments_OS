#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
struct threadArgs {
    unsigned int id;
    unsigned int numThreads;
};

void*child(void*params) {
    //struct threadArgs *args = (struct threadArgs*) params;
    //unsigned int childID = args->id;
    //unsigned int numThreads = args->numThreads;
    printf("Greetings from child #%u of %u\n", *(int*)params,*((int*)params));
}

int main(int argc, char**argv) {
    pthread_t*children; // dynamic array of child threads
    struct threadArgs*args; // argument buffer
    unsigned int numThreads = 5;
    // get desired # of threads
    if (argc > 1)
        numThreads = atoi(argv[1]);
    children = malloc(numThreads *sizeof(pthread_t)); // allocate array of handles
    for (unsigned int id = 0; id < numThreads; id++) {
        // create threads
        args = malloc(sizeof(struct threadArgs));
        args->id = id;
        args->numThreads = numThreads;
        int * id_1 = malloc(sizeof(int));
        *id_1 = id;
        pthread_create(&(children[id]), // our handle for the child
            NULL, // attributes of the child
            child, // the function it should run
            (void*)id_1); // args to that function
    }
    printf("I am the parent (main) thread.\n");
    for (unsigned int id = 0; id < numThreads; id++) {
        pthread_join(children[id], NULL );
    }
    free(children); // deallocate array
    return 0;
}