#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1

// struct for holding information specific to a given process
typedef struct {
  bool is_child_process;
  bool wraps_around_circle;
  int fd_read;
  int fd_write;
} process_specific_information;

// function declarations
int** create_pipe_array(int num_child_processes);
process_specific_information* collatz_circle_create(int num_child_processes, int **pipe_array);
void collatz_circle_loop(process_specific_information *ps_info);
int collatz_next_term(int previous_term);
void collatz_perform_cleanup(int num_child_processes, process_specific_information *ps_info, int **pipe_array);


int main(int argc, char *argv[]) {
  if(argc < 2) { // ensure number of child processes was given
    fprintf(stderr, "Error: must provide valid number of child processes\n");
    return 1;
  }

  int num_child_processes = atoi(argv[1]);
  if(num_child_processes < 0) { // ensure number of child processes is positive
    fprintf(stderr, "Error: number of child processes must be positive\n");
    return 1;
  }

  // create the pipe array 
  int **pipe_array = create_pipe_array(num_child_processes);
  if(pipe_array == NULL) {
    return 1;
  }

  // set up the collatz circle based on the desired number of children
  process_specific_information *ps_info = collatz_circle_create(num_child_processes, pipe_array);
  if(ps_info == NULL) {
    for(int i=0; i<num_child_processes; i++) {
      free(pipe_array[i]);
    }
    free(pipe_array);
    return 1;
  }

  // parental loop
  if(ps_info->is_child_process == false) {
    int initial_number;
    // core parental loop
    while(true) {
      // prompt the user for the intial number in the sequence & receive it
      printf("Enter first number in sequence:\n");
      scanf("%d", &initial_number);

      write(ps_info->fd_write, &initial_number, sizeof(int)); // write the number to the first child

      if(initial_number == 0) { // stop condition
        int status;
        for(int i=0; i<num_child_processes; i++) { // loop to wait for children to exit
          wait(&status);
        }
         collatz_perform_cleanup(num_child_processes, ps_info, pipe_array); // free allocated memory
        break;
      }
    }
  }
  else {
    collatz_circle_loop(ps_info); // execute the child loop
    collatz_perform_cleanup(num_child_processes, ps_info, pipe_array); // free allocated memory
    exit(0);
  }
  return 0;
}

int** create_pipe_array(int num_child_processes) {
  // create 2D array for pipes
  int **pipe_array = malloc(sizeof(int *) * num_child_processes); 
  if(pipe_array == NULL) {
    perror("memory allocation has failed");
    return NULL;
  }
  for(int i=0; i<num_child_processes; i++) {
    pipe_array[i] = malloc(sizeof(int) * 2);
    if(pipe_array[i] == NULL) {
      perror("memory allocation has failed");
      for(int j=0; j<i; j++) {
        free(pipe_array[j]);
      }
      free(pipe_array);
      return NULL;
    }
  }

  for(int i=0; i<num_child_processes; i++) { // create 2N pipe descriptors 
    if(pipe(pipe_array[i]) < 0) {
      perror("pipe creation failure");
      for(int j=0; j<num_child_processes; j++) {
        free(pipe_array[j]);
      }
      free(pipe_array);
      return NULL;
    }
  }
  return pipe_array;
}

process_specific_information* collatz_circle_create(int num_child_processes, int **pipe_array) {
  pid_t pid;
  process_specific_information *ps_info = malloc(sizeof(process_specific_information));
  
  if(ps_info == NULL) {
    perror("memory allocation failure");
    return NULL;
  }

  ps_info->is_child_process = false;
  ps_info->wraps_around_circle = false;
  
  for(int i=0; i<num_child_processes; i++) { // loop to create desired number of child processes
    if((pid = fork()) < 0) {
      perror("fork failure");
      free(ps_info);
      return NULL;
    }
    else if(pid == 0) {
      ps_info->is_child_process = true;
      // need to close all pipes not used by the given child
      for(int j=0; j<num_child_processes; j++) {
        if(j != i) { // close the read end of all pipes except i's (child 1 reads from p0, child 2 reads from p1...)
          close(pipe_array[j][READ]);
        }
        else { // this is the read file descriptor this child will use
          ps_info->fd_read = pipe_array[j][READ];
        }
        // close the write end of all pipes except i + 1's (need to make sure the final child process loops back around)
        if(j != (i + 1) % num_child_processes) {
          close(pipe_array[j][WRITE]);
        }
        else { // this is the write file descriptor this child will use
          ps_info->fd_write = pipe_array[j][WRITE];
          if(j == 0) {
            ps_info->wraps_around_circle = true;
          }
        }
      }
      break; // only the parent process should create children
    }
  }
  if(ps_info->is_child_process == false) { // close the proper pipes for the parent process
    ps_info->fd_write = pipe_array[0][WRITE];
    for(int i=0; i<num_child_processes; i++) {
      if(i != 0) { // close the write end of all pipes except the first one
        close(pipe_array[i][WRITE]); 
      }
      // close the read end of all pipes for the parent
      close(pipe_array[i][READ]);
    }
  }
  return ps_info;
}

void collatz_circle_loop(process_specific_information *ps_info) {
  int collatz_value;
  int odd_numbers_received = 0;
  int even_numbers_received = 0;
  // core child loop
  while(true) {
    usleep(1000000);

    printf("Child %d is ready\n", getpid());

    usleep(2500000);

    read(ps_info->fd_read, &collatz_value, sizeof(int));
    
    printf("Child %d has received: %d\n", getpid(), collatz_value);

    usleep(2500000);

    if(collatz_value == 0) {
      // prevent the last process from writing 0 back around
      if(!ps_info->wraps_around_circle) { 
        write(ps_info->fd_write, &collatz_value, sizeof(int));
      }
      close(ps_info->fd_read);
      close(ps_info->fd_write);
      printf("Child %d received %d even number(s)\n", getpid(), even_numbers_received);
      printf("Child %d received %d odd number(s)\n", getpid(), odd_numbers_received);
      printf("Child %d is done\n", getpid());
      break;
    }

    if(collatz_value % 2 == 0) {
      even_numbers_received++; // number is even, increment even counter
    }
    else {
      odd_numbers_received++; // number is odd, increment odd counter
    }

    if(collatz_value != 1) { // send to next child if number is not a 1
      collatz_value = collatz_next_term(collatz_value);
      write(ps_info->fd_write, &collatz_value, sizeof(int));
    }
  }
}

int collatz_next_term(int previous_term) {
  // calculate what the next number should be based on the collatz conjecture
  if(previous_term % 2 == 0) {
    return previous_term / 2;
  }
  return (previous_term * 3) + 1;
}

void collatz_perform_cleanup(int num_child_processes, process_specific_information *ps_info, int **pipe_array) {
  // free all allocated memory
  for(int i=0; i<num_child_processes; i++) {
    free(pipe_array[i]);
  }
  free(pipe_array);
  free(ps_info);
}