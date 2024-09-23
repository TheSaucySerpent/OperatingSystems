/*
Name: Skyler Burden
Course: Operating Systems - Section 1
Objective: Create a Simple Shell Program
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

// necessary define for determining acceptible input size
#define MAX_INPUT_SIZE 100
#define MAX_TOKENS (MAX_INPUT_SIZE + 1)

int main() {
  // cumulative variables that shouldn't reset each iteration of the loop
  int unknown_commands_count = 0; // the total number of unrecognized commands
  long total_children_nivcsw = 0; // the total number of invoulantary context switches (for all children)
  struct timeval total_children_utime = {.tv_sec = 0, .tv_usec = 0}; // the total user CPU time used
  char * prompt = malloc(sizeof(char) * MAX_INPUT_SIZE); // the prompt used (for extra credit)
  
  strcpy(prompt, "Enter desired command:"); // the default prompt

  while (true) {
      int token_count = 0; // a count of how many tokens the string was broken into
      char *user_input = malloc(sizeof(char) * MAX_INPUT_SIZE); // used to store user input
      char **user_tokens = malloc(sizeof(char *) * MAX_TOKENS); // used to store tokens

      // ensure malloc did not fail
      if (user_input == NULL || user_tokens == NULL) {
        perror("malloc failure");
        exit(1);
      }

      // prompt user for command and get input
      printf("%s ", prompt);
      fgets(user_input, MAX_INPUT_SIZE, stdin);

      // prompt again for input if nothing was entered (not counted in unknown commands total)
      if (user_input[0] == '\n' || user_input[0] == ' ') {
          free(user_input);
          free(user_tokens);
          continue;
      }

      // parse the command into tokens
      char * tok = strtok(user_input, " \n");
      while(tok != NULL && token_count < MAX_TOKENS - 1) {
        user_tokens[token_count++] = tok;
        tok = strtok(NULL, " \n");
      }
      // add NULL to the end of the tokens list (for execvp() call)
      user_tokens[token_count] = NULL;

      // break the loop if the entered command is quit
      if (strcmp(user_tokens[0], "quit") == 0) {
        free(user_input);
        free(user_tokens);
        break;
      }
      // implementing the built in prompt command (extra credit)
      else if (strcmp(user_tokens[0], "prompt") == 0) {
        if ((user_tokens[1] != NULL)) {
          strcpy(prompt, user_tokens[1]); // set new prompt based on user input
        }
        else {
          printf("prompt error: must specify desired prompt\n");  // not counting in unknown commands
        }
        free(user_input);
        free(user_tokens);
        continue;
      }
      // implementing the built in cd command (extra credit)
      else if (strcmp(user_tokens[0], "cd") == 0) {
        // special handling for if the user enters only cd or cd ~ (must ensure HOME env is not NULL)
        if ((user_tokens[1] == NULL || strcmp(user_tokens[1], "~") == 0) && (getenv("HOME") != NULL)) {
          chdir(getenv("HOME")); // change to home directory
        }
        // otherwise a filepath was given, ensure that changing to it succeeds
        else if (chdir(user_tokens[1]) < 0) {
          perror("chdir failure");
        };
        free(user_input);
        free(user_tokens);
        continue;
      }

      pid_t pid, child; // for holding process pid 
      int status; // for holding exit status
      struct rusage usage; // for holding resource usage information

      // fork a new process and ensure it succceded
      if ((pid = fork()) < 0) {
        free(user_input);
        free(user_tokens);
        free(prompt);
        perror("fork failure");
        exit(1);
      }
      // the child calls execvp() to run the command and exit()
      else if (pid == 0) {
        // exit the process if an unknown command was entered
        if (execvp(user_tokens[0], user_tokens) < 0) {
          free(user_input);
          free(user_tokens);
          free(prompt);
          perror("Unknown command");
          exit(2); 
        }
      }
      // the parent calls wait() to retrieve the child status
      else {
        child = waitpid(pid, &status, 0);

        // exit the process if the wait system call failed
        if (child < 1) {
          free(user_input);
          free(user_tokens);
          free(prompt);
          perror("waitpid error");
          exit(1);
        }
        // exit the process if the getrusage system call failed
        else if (getrusage(RUSAGE_CHILDREN, &usage) < 0) {
          free(user_input);
          free(user_tokens);
          free(prompt);
          perror("getrusage error");
          exit(1);
        }

        // calculate only recent child process usage information (since RUSAGE_CHILDREN is cumulative)
        time_t  child_process_utime_sec = usage.ru_utime.tv_sec - total_children_utime.tv_sec;
        suseconds_t child_process_utime_usec = usage.ru_utime.tv_usec - total_children_utime.tv_usec;
        long child_process_nivcsw = usage.ru_nivcsw - total_children_nivcsw;
        
        // output the user cpu time used and # of involuntary context switches for the previous process
        printf("User CPU time used: %ld.%06ld seconds\n", child_process_utime_sec,child_process_utime_usec);
        printf("# of involuntary context switches: %ld\n", child_process_nivcsw);

        // properly increment resource usage information
        total_children_nivcsw +=  child_process_nivcsw;
        total_children_utime.tv_sec += child_process_utime_sec;
        total_children_utime.tv_usec += child_process_utime_usec;

        // perform normal cleanup
        free(user_input);
        free(user_tokens);

        // increment the unknown commands counter if the exit status is 2
        if(WEXITSTATUS(status) == 2) {
          unknown_commands_count++;
        }
      }
  }

// output the total number of unknown commands entered (for extra-credit)
printf("# of unknown commands entered: %d\n", unknown_commands_count);

// free the prompt to prevent a memory leak
free(prompt);

return 0;
}