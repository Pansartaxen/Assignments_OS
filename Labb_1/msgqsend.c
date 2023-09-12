#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include<unistd.h>
#define PERMS 0644
struct my_msgbuf {
    long mtype;
    int mnums;
};

int main(void) {
    struct my_msgbuf buf;
    int msqid;
    int len;
    key_t key;
    srand(time(NULL));
    system("touch msgq.txt");

   if ((key = ftok("msgq.txt", 'B')) == -1) {
    /*Checks if the file exist, also why msgqracv should be launched after msgqsend*/
        perror("ftok");
        exit(1);
   }

   if ((msqid = msgget(key, PERMS | IPC_CREAT)) == -1) {
        perror("msgget");
        exit(1);
   }

    printf("Setup complete! Press enter to begin transmission\n");
    printf("Enter lines of text, ^D to quit:\n");
    buf.mtype = 2; /* we don't really care in this case */
    char keykey[5];

    while(strcmp(fgets(keykey, 5, stdin), "\n") == 1);
    for(int i = 0; i < 50; i++){
        buf.mnums = (int)rand();
        len = sizeof(buf.mnums);
        printf("Int #%d: %d\n", i, buf.mnums);
        if (msgsnd(msqid, &buf, len+1, 0) == -1) /* +1 for '\0' */
            perror("msgsnd");
    }

    //strcpy(buf.mnums, "end");
    //len = strlen(buf.mnums);
    sleep(0.1);
    if (msgsnd(msqid, &buf, len+1, 0) == -1) /* +1 for '\0' */
        perror("msgsnd");
        exit(1);

    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }
    printf("message queue: done sending messages.\n");
    return 0;
}