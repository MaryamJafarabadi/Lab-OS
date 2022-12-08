#include "types.h"
#include "stat.h"
#include "user.h"

int main()
{
    int pid1 = fork();
    if (pid1 == 0) 
    {
        printf(1, "We are in user mode and the system call of getting parent pid is being called!\n");
        printf(1, "second process pid: %d , parent: %d\n", getpid(), get_parent_pid());

        int pid2 = fork();
        if(pid2 == 0) 
        {
            printf(1, "third process pid: %d , parent: %d\n", getpid(), get_parent_pid());
        }
    }
    wait();
    exit();
}

