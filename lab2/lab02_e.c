#include <unistd.h>
#include <stdio.h>

int main() {
    // The list of args must end with a NULL
    // execl("/usr/bin/cal", "cal", "-3", "5", "2045", NULL);
    // execlp("cal", "some random string", "-3", "5", "2045", NULL);
    char* run_args[] = {"some random string", "-3", "5", "2045", NULL};
    execvp("cal", run_args);
    // The second arg is "ignored" by execl() or execlp(), it would not
    // affect MOST of the commands we run via exec__().
    // Typically the second arg is the name of the program
    // execl("/usr/bin/cal", "cal", "-3", "5", "2045", NULL);
    perror("After exec()");
    printf ("Just checking\n");
    return 0;
}