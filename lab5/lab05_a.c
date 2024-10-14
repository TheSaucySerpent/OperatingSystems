#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define FOO 4096

int main () {
    int shmId;
    char *shmPtr;

    struct shmid_ds buf; 
    key_t my_key = ftok("lab05_a.c", 5);

    if (my_key == -1) {
        perror("ftok failed");
        exit(1);
    }
    else {
        printf("ftok succeeded, key = %d\n", my_key);
    }
    if ((shmId =
         shmget (my_key, FOO,
                 IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
        perror ("i can't get no..\n");
        exit (1);
    }
    if ((shmPtr = shmat (shmId, NULL, 0)) == (void *) -1) {
        perror ("can't attach\n");
        exit (1);
    }
    printf("ID of shared memory segment: %d\n", shmId);
    printf ("value a: %lu\t value b: %lu\n", (unsigned long) shmPtr,
            (unsigned long) shmPtr + FOO);
    pause();
    if (shmdt (shmPtr) < 0) {
        perror ("just can't let go\n");
        exit (1);
    }
    if (shmctl (shmId, IPC_STAT, &buf) < 0) {
        perror ("can't access shared memory segment\n");
        exit (1);
    }
    printf("Size of segment: %zu\n", buf.shm_segsz);
    // if (shmctl (shmId, IPC_RMID, 0) < 0) {
    //     perror ("can't deallocate\n");
    //     exit (1);
    // }

    return 0;
}
