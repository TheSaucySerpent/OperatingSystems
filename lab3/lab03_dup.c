#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main() {
    char name[50];

    int fd = open("myoutput.txt", 
                    O_WRONLY | O_CREAT | O_EXCL,
                    S_IRUSR | S_IWUSR);
    if (errno != 0) {
        perror("Unable to create myoutput.txt");
        return 0;
    }
    printf ("Enter your name: ");
    fgets(name, 50, stdin);
    printf ("You entered: %s\n", name);

    printf ("Enter your neigbhor's name: ");
    fgets(name, 50, stdin);

    dup2(fd, fileno(stdout));
    printf ("Your neighbor is: %s\n", name);
    close (fd);
    return 0;
}