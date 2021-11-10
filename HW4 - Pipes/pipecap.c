#include <stdio.h>
#include <unistd.h>
#include<fcntl.h>
#include <errno.h>

#define WRITE_SIZE 128
int errno;

int main(){

    int p[2];

    if(pipe(p) < 0){
        perror("Pipe somehow failed");

        return -1;
    }

    fcntl(p[1], F_SETFL, O_NONBLOCK);

    char message[WRITE_SIZE];

    int i = 0;

    for(i = 0; i < WRITE_SIZE-1; i++){
        message[i] = 'a';
    }

    message[WRITE_SIZE-1] = '\0';

    int amount = 0;

    while(1){
        int i = write(p[1], message, WRITE_SIZE);

        if(errno != 0){
            printf("Breaking with error number: %d\n", errno); // EAGAIN is errno 11
            printf("Write system call failed after adding %d bytes.\n", amount);
            break;
        }
        else {
            amount += i;
        }
    }

    return 0;
}
