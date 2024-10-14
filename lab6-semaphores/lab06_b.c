#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>

typedef struct {
    int arr[2];
    sem_t mtx;
} shared_info;

int main (int argc, char*argv[]) {
    int status;
    long int i, loop = 0;
    shared_info *shr;
    int shmId;
    char shmName[50];
    pid_t pid;
    sprintf(shmName, "swap-%d", getuid());

    if(argc < 2) {
        fprintf(stderr, "Err: must provide number of times to loop\n");
        exit(1);
    }
   
    /*
     * TODO: get value of loop variable(from command - line
     * argument
     */
    loop = atoi(argv[1]);

    shmId = shm_open (shmName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmId, sizeof(shared_info));
    shr = mmap(NULL, sizeof(shared_info), PROT_READ|PROT_WRITE, MAP_SHARED, shmId, 0);

    shr->arr[0] = 0;
    shr->arr[1] = 1;

    // initialize named semaphore for threads with initial value 1
    sem_init(&shr->mtx, 1, 1);

    pid = fork ();
    if (pid == 0) {
        for (i = 0; i < loop; i++) {
            sem_wait(&shr->mtx); // lock the semaphore

            // TODO: swap the contents of arr[0] and arr[1]
            int temp_val = shr->arr[0];
            shr->arr[0] = shr->arr[1];
            shr->arr[1] = temp_val;

            sem_post(&shr->mtx); // unlock the semaphore
        }
        munmap (shr->arr, 2 * sizeof(long int));
        exit (0);
    }
    else {
        for (i = 0; i < loop; i++) {
            sem_wait(&shr->mtx); // lock the semaphore

            // TODO: swap the contents of arr[0] and arr[1]
            int temp_val = shr->arr[0];
            shr->arr[0] = shr->arr[1];
            shr->arr[1] = temp_val;

            sem_post(&shr->mtx); // unlock the semaphore
        }
    }

    wait (&status);
    printf ("values: %d\t%d\n", shr->arr[0], shr->arr[1]);
    munmap (shr->arr, sizeof(shared_info));
    shm_unlink(shmName);
    return 0;
}
