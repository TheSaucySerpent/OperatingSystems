#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1

int main() {
  pid_t pid1, pid2;
  int p1[2], p4[2];
  int value;

  pipe(p1);
  pipe(p4);

  if((pid1 = fork()) < 0) { // initial fork
    perror("fork failure");
    exit(1); 
  }
  else if(pid1 == 0) { // child process #1
    int p2[2];
    int read_value;

    pipe(p2); // pipe to send from child process #1 --> child process #2

    close(p1[WRITE]); // close write end of p1
    close(p2[READ]); // close read end of p2
    close(p4[READ]); // close read end of p4 (so that all children have read of p4 closed)

    read(p1[READ], &read_value, sizeof(int)); // read the value from the parent process

    printf("child process 1 sending %d to child process 2\n", read_value);
    // printf("PID: %d\n", getpid());

    write(p2[WRITE], &read_value, sizeof(int)); // write it to the next process (child process #2)

    close(p1[READ]);
    close(p2[WRITE]);

    if((pid2 = fork()) < 0) { // fork again within child process (to create one more pipe)
      perror("fork failure");
      exit(1);
    }
    else if(pid2 == 0) { // child process #2
      int p3[2];

      pipe(p3); // pipe to send from child process #2 --> child process #3

      close(p2[WRITE]); // close write end of p2
      close(p3[READ]); // close read end of p3

      read(p2[READ], &read_value, sizeof(int)); // read the value from child process #1

      printf("child process 2 sending %d to child process 3", read_value);

      write(p4[WRITE], &read_value, sizeof(int)); // write it to the next process (child process #3)

      close(p2[READ]);
      close(p3[WRITE]);

      exit(2);
    }
    exit(0);
      
    //   if((pid = fork()) < 0) { // fork again within child process (to create final pipe)
    //   perror("fork failure");
    //   exit(1);
    //   }
    //   else if(pid == 0) {
    //     close(p3[WRITE]); // close write end of p3

    //     read(p3[READ], &read_value, sizeof(int)); // read the value from child process #2

    //     printf("child process 3 sending %d to parent process", read_value);

    //     write(p4[WRITE], &read_value, sizeof(int)); // write the value back to the parent process

    //     close(p3[READ]);
    //     close(p4[WRITE]);
    //     exit(10);
    //   }
    // }
  }
  else {
    close(p1[READ]); // close low pressure end of p1
    close(p4[WRITE]); // close high pressure end of p4

    printf("Enter a number: ");
    scanf("%d", &value);
    write(p1[WRITE], &value, sizeof(int));

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    int final_value;
    read(p4[READ], &final_value, sizeof(int));

    close(p1[WRITE]);
    close(p4[READ]);

    printf("final value: %d\n", final_value);
    printf("status: %d\n", WEXITSTATUS(status));

    return 0;
    }
}
