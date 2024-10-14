#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

void* swapper(void*);

int arr[2];
sem_t mtx;

int main(int argc, char* argv[]) {
    pthread_t who;
    long int loop;

    if(argc < 2) {
        fprintf(stderr, "Err: must provide number of times to loop\n");
        exit(1);
    }

    // initialize unnamed semaphore for threads with initial value 1
    sem_init(&mtx, 0, 1);

    // TODO: get value of loop var (from command line arg)
    loop = atoi(argv[1]);

    arr[0] = 0;
    arr[1] = 1;
    pthread_create(&who, NULL, swapper, &loop);
    for (int k = 0; k < loop; k++) {
        sem_wait(&mtx); // lock the semaphore

        // TODO: swap the contents of arr[0] and arr[1]
        int temp_val = arr[0];
        arr[0] = arr[1];
        arr[1] = temp_val;

        sem_post(&mtx); // unlock the semaphore
    }
    int rc;
    pthread_join(who, (void **) &rc);
    printf ("Values: %5d %5d\n", arr[0], arr[1]);

    sem_destroy(&mtx); // release the semaphore
}

void* swapper(void *arg) {
    long int *num = (long int *) arg;
    for (int k = 0; k < *num; k++) {
        sem_wait(&mtx); // lock the semaphore

        // TODO: swap the contents of arr[0] and arr[1]
        int temp_val = arr[0];
        arr[0] = arr[1];
        arr[1] = temp_val;

        sem_post(&mtx); // unlock the semaphore
    }
    return 0;
}
