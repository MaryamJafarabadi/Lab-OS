#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
    int pid = atoi(argv[1]);
    int priority_ratio = atoi(argv[2]);
    int arrivaltime_ratio = atoi(argv[3]);
    int execcycle_ratio = atoi(argv[4]);
    BJF_parameter_process(pid, priority_ratio, arrivaltime_ratio, execcycle_ratio);
    exit();
}