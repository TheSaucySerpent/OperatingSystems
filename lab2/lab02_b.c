#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int i, limit;

    if (argc < 2) {
        fputs("Usage: must supply a limit value\n", stderr);
        exit(1);
    }
    limit = atoi(argv[1]);

    fork();
    fork();
    pid_t who = getpid();
    for (i = 0; i < limit; i++)
        printf("PID:%d %d\n", who, i);
    return 0;
}
