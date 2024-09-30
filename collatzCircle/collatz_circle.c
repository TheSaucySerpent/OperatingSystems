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
  int initial_value, calulcated_value, status;;

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

        read(p3[READ], &calulcated_value, sizeof(int)); // read the value from child process #2

        calulcated_value = collatz_next_term(calulcated_value);

        printf("child process 3 sending %d back to parent process\n", calulcated_value);

        write(p4[WRITE], &calulcated_value, sizeof(int)); // write calculated value to parent process

        // close used ends of pipes for best practice
        close(p3[READ]);
        close(p4[WRITE]);
        exit(0);
      }
    
      close(p2[WRITE]); // close write end of p2
      close(p3[READ]); // close read end of p3

      // close both ends of p1 and p4
      close(p1[READ]);
      close(p1[WRITE]);
      close(p4[READ]); 
      close(p4[WRITE]);

      read(p2[READ], &calulcated_value, sizeof(int)); // read the value from child process #1

      calulcated_value = collatz_next_term(calulcated_value);

      printf("child process 2 sending %d to child process 3\n", calulcated_value);

      write(p3[WRITE], &calulcated_value, sizeof(int)); // write it to the next process (child process #3)

      // close used ends of pipes for best practice
      close(p2[READ]);
      close(p3[WRITE]);

      wait(&status);
      exit(0);
    }

    close(p1[WRITE]); // close write end of p1
    close(p2[READ]); // close read end of p2

    // close both ends of p3 and p4
    close(p3[READ]);
    close(p3[WRITE]);
    close(p4[READ]); 
    close(p4[WRITE]);

    read(p1[READ], &calulcated_value, sizeof(int)); // read the value from the parent process

    calulcated_value = collatz_next_term(calulcated_value);

    printf("child process 1 sending %d to child process 2\n", calulcated_value);

    write(p2[WRITE], &calulcated_value, sizeof(int)); // write it to the next process (child process #2)

    // close used ends of pipes for best practice
    close(p1[READ]);
    close(p2[WRITE]); 

    wait(&status);
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
    scanf("%d", &initial_value);
    write(p1[WRITE], &initial_value, sizeof(int));

    // waitpid(pid1, &status, 0);
    wait(&status);

    int final_value;
    read(p4[READ], &final_value, sizeof(int));

    // close used ends of pipes for best practice
    close(p1[WRITE]);
    close(p4[READ]);

    printf("final value: %d\n", final_value);
    printf("status: %d\n", WEXITSTATUS(status));

    return 0;
    }
}

int collatz_next_term(int previous_term) {
  if(previous_term % 2 == 0) {
    return previous_term / 2;
  }
  return (previous_term * 3) + 1;
}
