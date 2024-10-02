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
  int collatz_fd_read;
  int collatz_fd_write;
  int report_to_parent_fd_write;
} process_specific_information;

// function declarations
int** create_pipe_array(int num_child_processes);
process_specific_information* collatz_circle_create(int num_child_processes, int **collatz_circle_pipe_array, int **report_to_parent_pipe_array);
void collatz_circle_loop(process_specific_information *ps_info);
int collatz_next_term(int previous_term);
void collatz_perform_cleanup(int num_child_processes, process_specific_information *ps_info, int **collatz_circle_pipe_array, int **report_to_parent_pipe_array);


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

  // create the pipe array for normal collatz circle communication
  int **collatz_circle_pipe_array = create_pipe_array(num_child_processes);
  if(collatz_circle_pipe_array == NULL) {
    return 1;
  }
  // create the pipe array for child processes to report to the parent (A-Level)
  int **report_to_parent_pipe_array = create_pipe_array(num_child_processes);
  if(report_to_parent_pipe_array == NULL) {
    for(int i=0; i<num_child_processes; i++) {
      free(collatz_circle_pipe_array[i]);
    }
    free(collatz_circle_pipe_array);
    return 1;
  }

  // set up the collatz circle based on the desired number of children
  process_specific_information *ps_info = collatz_circle_create(num_child_processes, collatz_circle_pipe_array, report_to_parent_pipe_array);
  if(ps_info == NULL) {
    for(int i=0; i<num_child_processes; i++) {
      free(collatz_circle_pipe_array[i]);
      free(report_to_parent_pipe_array[i]);
    }
    free(collatz_circle_pipe_array);
    free(report_to_parent_pipe_array);
    return 1;
  }

  // parental loop
  if(ps_info->is_child_process == false) {
    int initial_number; // stores the initial number entered
    pid_t ready_child_pid; // stores the pid of the child that has indicated it is ready
    int odd_numbers_received, even_numbers_received;

    // loop to wait for ready message from child before prompting for input
    for(int i=0; i<num_child_processes; i++) {
      read(report_to_parent_pipe_array[i][READ], &ready_child_pid, sizeof(pid_t));
      printf("Parent has recieved ready message from PID: %d\n", ready_child_pid);
    }

    // core parental loop
    while(true) {
      // prompt the user for the intial number in the sequence & receive it
      printf("Enter first number in sequence:\n");
      scanf("%d", &initial_number);

      // write the number to the first child
      write(ps_info->collatz_fd_write, &initial_number, sizeof(int));

      if(initial_number == 0) { // stop condition
        int status;
        // loop to report exit information and receive child status
        for(int i=0; i<num_child_processes; i++) {
          // receive pid and count of even/odd numbers received
          read(report_to_parent_pipe_array[i][READ], &ready_child_pid, sizeof(pid_t));
          read(report_to_parent_pipe_array[i][READ], &even_numbers_received, sizeof(int));
          read(report_to_parent_pipe_array[i][READ], &odd_numbers_received, sizeof(int));
          // close the read end for the pipe
          close(report_to_parent_pipe_array[i][READ]);

          printf("Child PID: %d Even numbers received: %d Odd numbers received: %d\n", 
          ready_child_pid, even_numbers_received, odd_numbers_received);

          wait(&status); // wait for children to exit
        }
        // close parent process's collatz fd_write (doesn't have  collatz fd_read)
        close(ps_info->collatz_fd_write); 

        // free allocated memory 
        collatz_perform_cleanup(num_child_processes, ps_info, collatz_circle_pipe_array, report_to_parent_pipe_array);

        break;
      }
    }
  }
  else {
    collatz_circle_loop(ps_info); // execute the child loop

    // free allocated memory
    collatz_perform_cleanup(num_child_processes, ps_info, collatz_circle_pipe_array, report_to_parent_pipe_array); 

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

process_specific_information* collatz_circle_create(int num_child_processes, int **collatz_circle_pipe_array, int **report_to_parent_pipe_array) {

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
    else if(pid == 0) { // setup child process
      ps_info->is_child_process = true;
      // need to close all pipes not used by the given child
      for(int j=0; j<num_child_processes; j++) {
        // close the read end of all pipes for reporting to parent
        close(report_to_parent_pipe_array[j][READ]);

        if(j != i) {
          // close the read end of all pipes except i's (child 1 reads from p0, child 2 reads from p1...)
          close(collatz_circle_pipe_array[j][READ]);
          // close the write end of all pipes except i's for reporting to parent
          close(report_to_parent_pipe_array[j][WRITE]);

        }
        else {
          // this is the read file descriptor this child will use
          ps_info->collatz_fd_read = collatz_circle_pipe_array[j][READ];
          // this is the write file descriptor this child will use to report to the parent
          ps_info->report_to_parent_fd_write = report_to_parent_pipe_array[j][WRITE];
        }

        // close the write end of all pipes except i + 1's (final child process loops back around)
        if(j != (i + 1) % num_child_processes) {
          close(collatz_circle_pipe_array[j][WRITE]);
        }
        else {
          // this is the write file descriptor this child will use
          ps_info->collatz_fd_write = collatz_circle_pipe_array[j][WRITE];
          if(j == 0) {
            // this is the child that wraps around the collatz circle
            ps_info->wraps_around_circle = true; 
          }
        }
      }
      break; // only the parent process should create children
    }
  }
  if(ps_info->is_child_process == false) { // close the proper pipes for the parent process
    // this is the only write fd the parent will use
    ps_info->collatz_fd_write = collatz_circle_pipe_array[0][WRITE]; 
    for(int i=0; i<num_child_processes; i++) {
      if(i != 0) { // close the write end of all pipes except the first one
        close(collatz_circle_pipe_array[i][WRITE]); 
      }
      close(collatz_circle_pipe_array[i][READ]); // close the read end of all pipes for the parent
      // close the write end of all pipes for reporting to parent (should only read)
      close(report_to_parent_pipe_array[i][WRITE]);
    }
  }
  return ps_info;
}

void collatz_circle_loop(process_specific_information *ps_info) {
  int collatz_value;
  int odd_numbers_received = 0;
  int even_numbers_received = 0;

  // send initial ready message to the parent
  pid_t pid = getpid();
  write(ps_info->report_to_parent_fd_write, &pid, sizeof(pid_t));

  // core child loop
  while(true) {
    usleep(1000000);

    printf("Child %d is ready\n", pid);

    usleep(2500000);

    read(ps_info->collatz_fd_read, &collatz_value, sizeof(int));
    
    printf("Child %d has received: %d\n", pid, collatz_value);

    usleep(2500000);

    if(collatz_value == 0) {
      // prevent the last process from writing 0 back around
      if(!ps_info->wraps_around_circle) { 
        write(ps_info->collatz_fd_write, &collatz_value, sizeof(int));
      }
      close(ps_info->collatz_fd_read); // close this process's read pipe
      close(ps_info->collatz_fd_write); // close this process's write pipe

      // write even & odd numbers recieved to the parent
      write(ps_info->report_to_parent_fd_write, &pid, sizeof(pid_t));
      write(ps_info->report_to_parent_fd_write, &even_numbers_received, sizeof(int));
      write(ps_info->report_to_parent_fd_write, &odd_numbers_received, sizeof(int));
      close(ps_info->report_to_parent_fd_write); // close the pipe to write to the parent

      printf("Child %d is done\n", pid);
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
      write(ps_info->collatz_fd_write, &collatz_value, sizeof(int));
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

void collatz_perform_cleanup(int num_child_processes, process_specific_information *ps_info, int **collatz_circle_pipe_array, int **report_to_parent_pipe_array) {
  // free all allocated memory
  for(int i=0; i<num_child_processes; i++) {
    free(collatz_circle_pipe_array[i]);
    free(report_to_parent_pipe_array[i]);
  }
  free(collatz_circle_pipe_array);
  free(report_to_parent_pipe_array);
  free(ps_info);
}