#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#define __NR_mysyscall 436

long mysyscall(void) {
    syscall(__NR_mysyscall);
}

int main() {
    long result = mysyscall();
    printf("Result: %d\n", result);
    return 0;
}
