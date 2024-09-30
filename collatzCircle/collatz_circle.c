#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1

// void core_collatz_loop(int fd_read, int fd_write);
// void close_undesired_pipes(int fd_count, int *fd_array);
int collatz_next_term(int previous_term);

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
  
  int num_pipes = num_child_processes + 1;
  int p[num_pipes][2]; // 2D array for pipes
  for(int i=0; i<num_pipes; i++) { // create 2N pipe descriptors 
    if(pipe(p[i]) < 0) {
      perror("pipe creation failure");
      return 1;
    }
  }

  pid_t pid;
  int fd_read, fd_write; // file descriptors that the given process will use
  for(int i=0; i<num_child_processes; i++) { // loop to create desired number of child processes
    if((pid = fork()) < 0) {
      perror("fork failure");
    }
    else if(pid == 0) {
      // need to close all pipes not used by the given child
      for(int j=0; j<num_pipes; j++) {
        if(j != i) { // close the read end of all pipes except i's (child 1 reads from p0, child 2 reads from p1...)
          close(p[j][READ]);
        }
        else { // this is the read file descriptor this child will use
          fd_read = p[j][READ];
        }
        // close the write end of all pipes except i + 1's (child 1 writes to p1, child 2 writes to p3...)
        if(j != (i + 1)){
          close(p[j][WRITE]);
        }
        else { // this is the write file descriptor this child will use
          fd_write = p[j][WRITE];
        }
      }
      break; // only the parent process should create children
    }
  }
  if(pid != 0) {
    for(int i=0; i<num_pipes; i++) {
      if(i != 0) { // close the write end of all pipes except the first one
        close(p[i][WRITE]); 
      }
      // close the read end of all pipes for the parent
      close(p[i][READ]);
    }
    fd_write = p[0][WRITE];

    int initial_number;
    // core parental loop
    while(true) {
      printf("Enter first number in sequence: ");
      scanf("%d", &initial_number);

      write(fd_write, &initial_number, sizeof(int));

      if(initial_number == 0) { // stop condition
        break;
      }
    }
  }
  else {
    int received_number, sent_number;
    // core child loop
    while(true) {
      // printf("Child %d is ready\n", getpid());

      read(fd_read, &received_number, sizeof(int));

      printf("Child %d has received: %d\n", getpid(), received_number);

      if(received_number != 1) {
        sent_number = collatz_next_term(received_number);
        write(fd_write, &sent_number, sizeof(int));
      }
    }
  }


  





  return 0;
}

// void core_collatz_loop(int fd_read, int fd_write) {
//   int collatz_value;
//   while(true) {
//     usleep(2500000);
//     printf("Child %d is ready\n", getpid());
//     read(fd_read, &collatz_value, sizeof(int)); // read the value from child process #2

//     usleep(2500000);
//     printf("PID: %d has read %d\n", getpid(), collatz_value);

//     if(collatz_value != 0 || collatz_value != 1) {
//       collatz_value = collatz_next_term(collatz_value);
//     }
//     write(fd_write, &collatz_value, sizeof(int)); // write calculated value to parent process

//     if(collatz_value == 0) {
//       break;
//     }
//   }
//   // close used ends of pipes for best practice
//   close(fd_read);
//   close(fd_write);
//   return;
// }

// void close_undesired_pipes(int fd_count, int *fd_array) {
//   for(int i=0; i<fd_count; i++) {
//     close(fd_array[i]);
//   }
//   return;
// }


int collatz_next_term(int previous_term) {
  if(previous_term % 2 == 0) {
    return previous_term / 2;
  }
  return (previous_term * 3) + 1;
}
