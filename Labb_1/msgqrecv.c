#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include<unistd.h>
#define PERMS 0644

struct my_msgbuf {
    long mtype;
    int mtext;
};

int main(void) {
    struct my_msgbuf buf;
    int msqid;
    int toend;
    key_t key;
    buf.mtype = 2;
    if ((key = ftok("msgq.txt", 'B')) == -1) {
        perror("ftok");
        exit(1);
    }
    if ((msqid = msgget(key, PERMS)) == -1) { /* connect to the queue */
        perror("msgget");
        exit(1);
    }
    printf("Setup done: ready to receive messages.\n");
    for(int i = 0; i < 50; i++) { /* normally receiving never ends but just to make conclusion */
        /* this program ends with string of end */
        if (msgrcv(msqid, &buf, sizeof(buf.mtext)*50, 0, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
        printf("Integer #%d: %d\n", i, buf.mtext);
        fflush(stdout);
    }
    printf("message queue: done receiving messages.\n");
    sleep(0.1);
    system("rm msgq.txt");
    return 0;
}