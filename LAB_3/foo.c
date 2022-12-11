#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
   int pid;
    float x = 1000000;
    for (int i = 0 ; i < 5 ; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            for (long int j = 0 ; j < 3000000000 ; j++)
            {
                x = x / 1.001;
            }
            exit();
        }
    }
    for (int i = 0; i < 5; i++)
      wait();
    return 0;
}