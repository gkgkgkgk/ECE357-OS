#include "sem.h"
#include <stdio.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SHELL_COUNT 3
#define TASK_COUNT 6

int main(int argc, char* argv[]){

    int rockCount = 1;
    int moveCount = rockCount;

    rockCount = atoi(argv[1]);
    moveCount = atoi(argv[2]);

    struct sem* shells = (struct sem*)mmap(NULL, sizeof(struct sem)*3, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);

    int i, j;
    for(i = 0; i < SHELL_COUNT; i++){
        sem_init(shells+i,rockCount);
    }


    int pid = getpid();
    
    // parent variables
    int children_pid[TASK_COUNT];
    
    // child variables
    int my_procnum = 0;
    int my_pid;    
    int fromShells[] = {0, 0, 1, 1, 2, 2};
    int toShells[] = {1, 2, 0, 2, 0, 1};

    for(i = 0; i < TASK_COUNT && pid != 0; i++){
        pid = fork();
        switch(pid){
            case -1:
                perror("Error with fork");
                exit(errno);

            case 0:
                my_procnum = i+1;
                my_pid = getpid();
                fprintf(stderr, "VCPU %d starting, pid %d\n", my_procnum, my_pid);
                break;

            default:
                children_pid[i] = pid;
                break;
        }
    }

    if(pid == 0){
        for(i = 0; i < moveCount; i++){
            // printf("CHILD %d: About to move from shell %d count: %d ", my_procnum, fromShell, shells[fromShell].count);
            // printf("to shell %d count: %d\n", toShell, shells[toShell].count);
            
            if(sem_try(shells+fromShells[my_procnum-1]) == 1){ // successfully removed a shell
                sem_inc(shells + toShells[my_procnum-1]);
            }
            else{
                sem_wait(shells + fromShells[my_procnum-1], my_procnum);
                sem_inc(shells + toShells[my_procnum-1]);
            }

            // printf("CHILD %d: Moved from shell %d count: %d ", my_procnum, fromShell, shells[fromShell].count);
            // printf("to shell %d count: %d\n", toShell, shells[toShell].count);
        }
        fprintf(stderr, "\nVCPU %d done\n\n",my_procnum);
    }
    else{    
        printf("Main Process Spawned all children, waiting");
        for(i = 0; i < TASK_COUNT; i++){
            waitpid(children_pid[i], NULL, 0);
        }

        for(i = 0; i < SHELL_COUNT; i++){
            printf("\nShell #%d Count: %d\n", i+1, shells[i].count);
            for(j = 0; j < TASK_COUNT; j++){
                printf("VCPU %d: sleeps: %d wakes: %d handlers: %d\n", j+1, shells[i].sleep_count[j+1], shells[i].woken_count[j+1], shells[i].handle_count[j+1]);
            }
            printf("\n");
        }
    }

    return 0;

}