#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  char input1[] = "A   test\ninput string";

  char *tok;
  printf ("Using strtok() on %s\n", input1);
  // On the initial call to strtok(), its first arg is the input string
  // the delimiter is a SPACE or a NEWLINE
  tok = strtok(input1, " \n");
  while (tok != NULL) {
    printf ("[%s]\n", tok);
    // On subsequent calls to strtok(), its first arg is NULL
    tok = strtok(NULL, " \n");
  }
  return 0;
}