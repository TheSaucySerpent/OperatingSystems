#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <sys/types.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1
#define MAX 1024

int main () {
    int fd[2]; // pipe for sending from child to parent process (Q1)
    int bd[2]; // pipe for sending from parent to child process (Q2)
    size_t num, len;
    pid_t pid;
    char str[MAX];
    char tokenized_str[MAX];
    int a_count;

    if (pipe (fd) < 0 || pipe(bd) < 0) {
        perror ("plumbing problem");
        // close both pipes completely for best practice
        close (fd[READ]);
        close (fd[WRITE]);
        close (bd[READ]);
        close (bd[WRITE]);
        exit (1);
    }
    printf("Pipe descriptors: read=%d write=%d\n", fd[0], fd[1]);
    // point A

    if ((pid = fork ()) < 0) {
        perror ("fork failed");
        // close both pipes completely for best practice
        close (fd[READ]);
        close (fd[WRITE]);
        close (bd[READ]);
        close (bd[WRITE]);
        exit (1);
    }
    // point B
    else if (pid == 0) {
        close (fd[READ]); // close undesired end of pipe
        close (bd[WRITE]); // close undesired end of pipe

        while(true) {
            //point C
            printf ("Type a sentence: ");
            fgets (str, MAX, stdin);
            printf ("Sent by %d: %s", getpid(), str);
            len = strlen(str) + 1;
            write (fd[WRITE], &len, sizeof(len));
            write (fd[WRITE], (const void *) str, (size_t) len);

            strcpy(tokenized_str, str);
            char *token = strtok(tokenized_str, " \n");
            // break from the loop if quit is entered (and skip counting a's)
            if(token != NULL && strcmp(token, "quit") == 0) { 
                break;
            }
            
            num = read (bd[READ], &a_count, sizeof(a_count)); // read a_count from the pipe
            if(num != sizeof(a_count)) {
                close (fd[WRITE]); // close write end of the pipe for best practice
                close (bd[READ]); // close read end of the pipe for best practice
                exit (1);
            }
            printf("Number of a's in string (not case sensitive): %d\n", a_count);
        }
        close (fd[WRITE]); // close write end of the pipe for best practice
        close (bd[READ]); // close read end of the pipe for best practice
        exit (0);
    }

    close (fd[WRITE]); // close undesired end of pipe
    close (bd[READ]); // close undesired end of pipe

    while(true) {
        //point C
        read(fd[READ], &len, sizeof(len));
        perror ("pipe read string length\n");
        num = read (fd[READ], (void *) str, len);
        perror ("pipe read string\n");
        if (num != len) {
            close (fd[READ]); // close read end of the pipe for best practice
            close (bd[WRITE]); // close write end of the pipe for best practice
            exit (1);
        }
        printf ("Received by %d: %s", getpid(), str);

        strcpy(tokenized_str, str);
        char *token = strtok(tokenized_str, " \n");
        // break from the loop if quit is entered (and skip counting a's)
        if(token != NULL && strcmp(token, "quit") == 0) { 
            break;
        }

        a_count = 0;
        // calculate the number of a's in the string (not case-sensitive)
        for(int i=0; i<len - 1; i++) {
            if(str[i] == 'a' || str[i] == 'A') {
                a_count++;
            }
        }
        write (bd[WRITE], &a_count, sizeof(a_count)); // write a_count to the pipe
    }
    close (fd[READ]); // close read end of the pipe for best practice
    close (bd[WRITE]); // close write end of the pipe for best practice
    
    int status;
    pid = wait(&status); // add wait call to parent process

    return 0;
}
