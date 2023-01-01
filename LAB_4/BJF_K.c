#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
    //int priority_ratio = ;
    //int arrivaltime_ratio = ;
    //int execcycle_ratio = ;
    BJF_parameter_kernel(atoi(argv[1]), atoi(argv[2]),atoi(argv[3]));
    exit();
}