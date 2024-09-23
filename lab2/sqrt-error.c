// Be sure to link this program with the math library (-lm)
#include <stdio.h>
#include <math.h>
#include <errno.h>

int main() {
    double x, z;

    printf ("Enter a number: ");
    scanf ("%lf", &x);
    z = sqrt(x);
    if (errno != 0)
        perror("Bad result");
    else {
        perror("Normal operation");
        printf ("Result is %.2f\n", z);
    }
    return 0;
}
