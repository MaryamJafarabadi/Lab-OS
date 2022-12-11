#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
    int pid = atoi(argv[1]);
    int dest_queue = atoi(argv[2]);
    change_process_queue(pid, dest_queue);
    exit();
}

