#include <stdio.h>

long read() {
    long x;
    scanf("%ld", &x);
    return x;
}

void write(long x) {
    printf("%ld\n", x);
}
