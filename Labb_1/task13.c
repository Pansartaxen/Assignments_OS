/***************************************************************************
*
*Sequential version of Matrix−Matrix multiplication
*
• Parallelize only the matrix multiplication, but not the initialization.
• A new thread shall be created for each row to be calculated in the matrix.
• The row number of the row that a thread shall calculate, shall be passed to the new thread as a parameter to the new
thread.
• The main function shall wait until all threads have terminated before the program terminates.
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#define SIZE 1024
static double a[SIZE][SIZE];
static double b[SIZE][SIZE];
static double c[SIZE][SIZE];

clock_t start, end;
double cpu_time_used;

struct threadArgs
{
    /*Struct for sending the ID of every professor as argument
      to prof function*/
    unsigned int id;
};

static void init_matrix(void)
{
    int i, j;
    for (i = 0; i < SIZE; i++)
        for (j = 0; j < SIZE; j++) {
            /*Simple initialization, which enables us to easy check
            *the correct answer. Each element in c will have the same
            *value as SIZE after the matmul operation.
            */
            a[i][j] = 1.0;
            b[i][j] = 1.0;
        }
}

static void * matmul_seq(void *row)
{
    int i = *(int*)row;
    for (int j = 0; j < SIZE; j++) {
        c[i][j] = 0.0;
        for (int k = 0; k < SIZE; k++)
            c[i][j] = c[i][j] + a[i][k] * b[k][j];
    }
    return NULL;
}

static void OGmatmul_seq(void)
{
    int i, j, k;
    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            c[i][j] = 0.0;
            for (k = 0; k < SIZE; k++)
                c[i][j] = c[i][j] + a[i][k] * b[k][i];
        }
    }
}

static void print_matrix(void)
{
    int i, j;
    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++)
            printf(" %7.2f", c[i][j]);
        printf("\n");
    }
}

int main(int argc, char **argv)
{

    struct timespec start, finish;
    double dif;

    clock_gettime(CLOCK_MONOTONIC, &start);

    init_matrix();
    pthread_t *children;
    children = malloc(SIZE * sizeof(pthread_t));
    struct threadArgs *args;
    args = malloc(SIZE * sizeof(struct threadArgs));

    for (int i = 0; i < SIZE; i++)
    {
        args[i].id = i;
        pthread_create(&(children[i]), NULL, matmul_seq, (void *)&args[i]);
    }

    for (int id = 0; id < SIZE; id++)
    {
        pthread_join(children[id], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &finish);

    dif = (finish.tv_sec - start.tv_sec);
    dif += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    printf("Time used multithread: %.2lf seconds\n", dif);
    double mul_dif = dif;

    clock_gettime(CLOCK_MONOTONIC, &start);
    OGmatmul_seq();
    clock_gettime(CLOCK_MONOTONIC, &finish);

    dif = (finish.tv_sec - start.tv_sec);
    dif += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("Time used sequential: %.2lf seconds\n", dif);
    printf("Speedup: %.2lf\n", dif / mul_dif);
    free(children);
    return 0;
}