#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf(1,"Please enter 1 number.\n");
        exit();
    }
    int saved_reg;
    asm ("movl %%ebp , %0;" :"=r" (saved_reg) ::);
    asm ("movl %0, %%ebp;" :: "r"(atoi(argv[1])):);
    printf(1,"We are in user mode and the system call of finding the largest prime factor is being called!\n");
    int result = find_largest_prime_factor();
    if (result == -2) printf(1, "Please enter positive number.\n");
    else if (result == -1) printf(1,"This number has no prime factor.\n");
    else printf(1,"%d\n", result);
    asm("movl %0, %%ebp" : : "r"(saved_reg));
    exit();
}
