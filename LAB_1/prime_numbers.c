#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]){
    int first_num = atoi(argv[1]);
    int second_num = atoi(argv[2]);

    if(first_num > second_num){
        int temp = first_num;
        first_num = second_num;
        second_num = temp;
    }
    int isPrime;
    int index = 0;
    int result[800];

    for(int i = first_num + 1; i < second_num; i++){
        isPrime = 1;
        for(int j = 2; j <= i / 2; ++j){ 
            if(i % j == 0){
                isPrime = 0;
                break;
            }
        }
        if(isPrime == 1){
            result[index] = i;
            index++;
        }
    }

    unlink("prime_numbers.txt");
    int fd = open("prime_numbers.txt", O_CREATE | O_RDWR);
    for(int i = 0; i < index; i++){
        if(i == index - 1)
            printf(fd, "%d\n", result[i]);
        else
            printf(fd, "%d ", result[i]);
    }
    close(fd);
    exit();
}