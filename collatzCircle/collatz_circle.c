#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1

void core_collatz_loop(int fd_read, int fd_write);
void close_undesired_pipes(int fd_count, int *fd_array);
int collatz_next_term(int previous_term);

int main() {
  pid_t pid1, pid2, pid3;
  int p1[2], p2[2], p3[2], p4[2];
  int collatz_value, status;
  int fd_count = 6;

  pipe(p1); // pipe to send from parent to child process #1
  pipe(p2); // pipe to send from child process #1 to child process #2
  pipe(p3); // pipe to send from child process #2 to child process #3
  pipe(p4); // pipe to send from child process #3 back to the parent

  if((pid1 = fork()) < 0) { // initial fork (+1 process)
    perror("fork failure");
    exit(1); 
  }
  else if(pid1 == 0) { // child process #1
    if((pid2 = fork()) < 0) { // secondary fork (+1 process)
      perror("fork failure");
      exit(1);
    }
    else if(pid2 == 0) { // child process #2
      if((pid3 = fork()) < 0) { // third fork (+1 process)
        perror("fork failure");
        exit(1);
      }
      else if(pid3 == 0) { // child process #3
        int fd_array[] = {p3[WRITE], p4[READ], p1[READ], p1[WRITE], p2[READ], p2[WRITE]};
        close_undesired_pipes(fd_count, fd_array);

        core_collatz_loop(p3[READ], p4[WRITE]);

        printf("Child %d is done\n", getpid());
        exit(0);
      }
    
      int fd_array[] = {p2[WRITE], p3[READ], p1[READ], p1[WRITE], p4[READ], p4[WRITE]};
      close_undesired_pipes(fd_count, fd_array);

      core_collatz_loop(p2[READ], p3[WRITE]);

      wait(&status);
      printf("Child %d is done\n", getpid());
      exit(0);
    }
    int fd_array[] = {p1[WRITE], p2[READ], p3[READ], p3[WRITE], p4[READ], p4[WRITE]};
    close_undesired_pipes(fd_count, fd_array);

    core_collatz_loop(p1[READ], p2[WRITE]);

    wait(&status);
    printf("Child %d is done\n", getpid());
    exit(0);
  }
  else {
    int fd_array[] = {p1[READ], p4[WRITE], p2[READ], p2[WRITE], p3[READ], p3[WRITE]};
    close_undesired_pipes(fd_count, fd_array);

    printf("Enter a number: ");
    scanf("%d", &collatz_value);
    while(true) {

      write(p1[WRITE], &collatz_value, sizeof(int)); // write the initial value to child process #1

      read(p4[READ], &collatz_value, sizeof(int));
      usleep(2500000);
      printf("Parent Process PID: %d has read %d\n", getpid(), collatz_value);

      if(collatz_value == 1) {
        printf("final value: %d\n", collatz_value);
        write(p1[WRITE], &collatz_value, sizeof(int)); // write 1 to child process #1

        printf("Enter a number: ");
        scanf("%d", &collatz_value);
        continue;
      }
      else if(collatz_value == 0) { // break from the loop
        close(p1[WRITE]);
        close(p4[READ]);
        printf("Parent %d is done\n", getpid());
        break;
      }
      // printf("status: %d\n", WEXITSTATUS(status));
    }

    // close used sides of pipe for best practice
    close(p1[WRITE]);
    close(p4[READ]);
    wait(&status);
    return 0;
  }
}

void core_collatz_loop(int fd_read, int fd_write) {
  int collatz_value;
  while(true) {
    usleep(2500000);
    printf("Child %d is ready\n", getpid());
    read(fd_read, &collatz_value, sizeof(int)); // read the value from child process #2

    usleep(2500000);
    printf("PID: %d has read %d\n", getpid(), collatz_value);

    if(collatz_value != 0 || collatz_value != 1) {
      collatz_value = collatz_next_term(collatz_value);
    }
    write(fd_write, &collatz_value, sizeof(int)); // write calculated value to parent process

    if(collatz_value == 0) {
      break;
    }
  }
  // close used ends of pipes for best practice
  close(fd_read);
  close(fd_write);
  return;
}

void close_undesired_pipes(int fd_count, int *fd_array) {
  for(int i=0; i<fd_count; i++) {
    close(fd_array[i]);
  }
  return;
}


int collatz_next_term(int previous_term) {
  if(previous_term % 2 == 0) {
    return previous_term / 2;
  }
  return (previous_term * 3) + 1;
}
