#include <stdio.h>

int main(int argc, char* argv[]) {
        for (int k = 0; k < argc; k++) {
                    printf ("Arg-%d is [%s]\n", k, argv[k]);
                        }
            return 0;
}
