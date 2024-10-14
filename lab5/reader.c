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

int main(int argc, char* argv[]) {

  // ensure it was specified which reader this is
  if(argc < 2) {
    fprintf(stderr, "Err: must specify if 1st or 2nd reader\n");
    exit(1);
  }
  int reader_index = atoi(argv[1]) - 1; // calculate proper reader_index

  int shmId;
  IPC_DATA *shared_data;
  key_t my_key = ftok("writer.c", 1); // create same unique key as writer

  if(my_key == -1) {
    perror("ftok failed"); // ensure key creation was successful
    exit(1);
  }
  if((shmId = shmget(my_key, sizeof(IPC_DATA), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0) {
    perror("shmget error"); // ensure shmget was successful
    exit(1);
  }
  if((shared_data = shmat(shmId, NULL, 0)) == (void *) -1) {
    perror("shmat error"); // ensure shmat was successful
    exit(1);
  }

  while(true) {
    shared_data->readers_done[reader_index] = false; // mark appropraite reader as not done reading

    while(!shared_data->writer_done) {} // prevent reader from entering critical section until writer is done
    
    fputs(shared_data->user_input, stdout); // read the message

    if(strcmp(shared_data->user_input, "quit\n") == 0) {
      break; // exit if user entered quit
    }

    shared_data->readers_done[reader_index] = true; // mark appropriate reader as done reading

    // wait for writer to say that it is not done before continuing (should ensure message prints only once and other busy loop is not bypassed)
    while (shared_data->writer_done) {}
  }
  shared_data->readers_done[reader_index] = true; // mark appropriate reader as done reading if quit was entered

  return 0;
}