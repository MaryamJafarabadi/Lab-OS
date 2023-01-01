#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
    int pid = atoi(argv[1]);
    int ticket = atoi(argv[2]);
    lottery_ticket(pid, ticket);
    exit();
}