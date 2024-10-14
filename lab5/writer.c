#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

// allowed size of user input (arbitrary number)
#define MAX_INPUT_SIZE 5000

// struct for shared data segment (best practice)
typedef struct {
    char user_input[MAX_INPUT_SIZE];
    bool writer_done;
    bool readers_done[2];
  } IPC_DATA;

int main() {
  int shmId;
  IPC_DATA *shared_data;
  key_t my_key = ftok("writer.c", 1); // create unique key

  if(my_key == -1) {
    perror("ftok failed"); // ensure key creation was successful
    exit(1);
  }
  if((shmId = shmget(my_key, sizeof(IPC_DATA), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0) {
    perror("shmget error"); // ensure shmget was successful
    exit(1);
  }
  if((shared_data = shmat(shmId, NULL, 0)) == (void *) -1) {
    perror("shmat error"); // ensure shamat was successful
    exit(1);
  }

  while(true) {
    printf("Enter desired message: "); // prompt user for input
    fgets(shared_data->user_input, MAX_INPUT_SIZE, stdin); // get user input

    if(strcmp(shared_data->user_input, "quit\n") == 0) {  
      break; // exit if user entered quit
    }
    shared_data->writer_done = true; // mark writer as done

    while(!shared_data->readers_done[0] || !shared_data->readers_done[1]) {} // wait for both readers to be finished

    shared_data->writer_done = false; // mark writer as not done (going to write again)
  }

  // ensure that the readers are done before deallocation
  shared_data->writer_done = true; // mark writer as done
  while(!shared_data->readers_done[0] || !shared_data->readers_done[1]) {} // wait for both readers to be finished

  if (shmctl (shmId, IPC_RMID, 0) < 0) {
      perror ("can't deallocate"); // ensure deallocation with shmctl was successful
      exit (1);
  }
  return 0;
}