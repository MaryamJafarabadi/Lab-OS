#include "types.h"
#include "stat.h"
#include "user.h"

void phil(int philsoph)
{
    sem_acquire(5);
    sem_acquire(philsoph - 1);
    sem_acquire(philsoph % 5);
    sleep(5);
    sem_release(philsoph - 1);
    sem_release(philsoph % 5);
    sem_release(5);
    sleep(5);
}

int main()
{
    sem_init(0, 1);
    sem_init(1, 1);
    sem_init(2, 1);
    sem_init(3, 1);
    sem_init(4, 1);
    sem_init(5, 4);

    for(int i = 0 ; i < 5 ; i++)
    {
        int id = fork();
        if(id==0){
            phil(i+1);
            exit();
        }
    }
    while(wait() != -1);
    exit();
}