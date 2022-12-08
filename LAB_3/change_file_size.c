#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) 
{
    if(argc != 3) 
    {
        printf(1,"Please enter path and file size that you want.\n");
        exit();
    }
    change_file_size(argv[1], atoi(argv[2]));
    exit();
}

