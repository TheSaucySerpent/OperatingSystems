#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1

int collatz_next_term(int previous_term);

int main() {
  pid_t pid1, pid2, pid3;
  int p1[2], p2[2], p3[2], p4[2];
  int collatz_value, status;

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
        close(p3[WRITE]); // close write end of p3
        close(p4[READ]); // close read end of p4

        // close both ends of p1 and p2
        close(p1[READ]);
        close(p1[WRITE]);
        close(p2[READ]);
        close(p2[WRITE]);

        while(true) {
          usleep(2500000);
          printf("Child %d is ready\n", getpid());
          read(p3[READ], &collatz_value, sizeof(int)); // read the value from child process #2

          usleep(2500000);
          printf("PID: %d has read %d\n", getpid(), collatz_value);

          if(collatz_value != 1) {
            collatz_value = collatz_next_term(collatz_value);
          }
          write(p4[WRITE], &collatz_value, sizeof(int)); // write calculated value to parent process

          if(collatz_value == 0) {
            break;
          }
        }
        // close used ends of pipes for best practice
        close(p3[READ]);
        close(p4[WRITE]);
        printf("Child %d is done\n", getpid());
        exit(0);
      }
    
      close(p2[WRITE]); // close write end of p2
      close(p3[READ]); // close read end of p3

      // close both ends of p1 and p4
      close(p1[READ]);
      close(p1[WRITE]);
      close(p4[READ]); 
      close(p4[WRITE]);

      while(true) {
        usleep(2500000);
        printf("Child %d is ready\n", getpid());
        read(p2[READ], &collatz_value, sizeof(int)); // read the value from child process #1

        usleep(2500000);
        printf("PID: %d has read %d\n", getpid(), collatz_value);

        if(collatz_value != 1) {
          collatz_value = collatz_next_term(collatz_value);
        }
        write(p3[WRITE], &collatz_value, sizeof(int)); // write it to the next process (child process #3)
        
        if(collatz_value == 0) {
          break;
        }
      }
      // close used ends of pipes for best practice
      close(p2[READ]);
      close(p3[WRITE]);
      wait(&status);
      printf("Child %d is done\n", getpid());
      exit(0);
    }

    close(p1[WRITE]); // close write end of p1
    close(p2[READ]); // close read end of p2

    // close both ends of p3 and p4
    close(p3[READ]);
    close(p3[WRITE]);
    close(p4[READ]); 
    close(p4[WRITE]);

    while(true) {
      usleep(2500000);
      printf("Child %d is ready\n", getpid());
      read(p1[READ], &collatz_value, sizeof(int)); // read the value from the parent process

      usleep(2500000);
      printf("PID: %d has read: %d\n", getpid(), collatz_value);

      if(collatz_value != 1) { 
        collatz_value = collatz_next_term(collatz_value);
      }
      write(p2[WRITE], &collatz_value, sizeof(int)); // write it to the next process (child process #2)

      if(collatz_value == 0) {
        break;
      }
    }
    // close used ends of pipes for best practice
    close(p1[READ]);
    close(p2[WRITE]);
    wait(&status);
    printf("Child %d is done\n", getpid());
    exit(0);
  }
  else {
    close(p1[READ]); // close low pressure end of p1
    close(p4[WRITE]); // close high pressure end of p4

    // close both ends of p2 and p3
    close(p2[READ]);
    close(p2[WRITE]);
    close(p3[READ]);
    close(p3[WRITE]);

    printf("Enter a number: ");
    scanf("%d", &collatz_value);
    while(true) {
      if(collatz_value == 0) { // break from the loop
        close(p1[WRITE]);
        close(p4[READ]);
        break;
      }
      if(collatz_value != 1) {
        collatz_value = collatz_next_term(collatz_value);
      }
      write(p1[WRITE], &collatz_value, sizeof(int)); // write the initial value to child process #1

      read(p4[READ], &collatz_value, sizeof(int));

      usleep(2500000);
      printf("Parent process sending %d to child process 1\n", collatz_value);

      if(collatz_value == 1) {
        printf("final value: %d\n", collatz_value);
        write(p1[WRITE], &collatz_value, sizeof(int)); // write 1 to child process #1

        printf("Enter a number: ");
        scanf("%d", &collatz_value);
        continue;
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

// void core_collatz_loop() {

// }

int collatz_next_term(int previous_term) {
  if(previous_term % 2 == 0) {
    return previous_term / 2;
  }
  return (previous_term * 3) + 1;
}
