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

void assignShell(int* fromShell, int* toShell){
    *toShell = (*toShell + 1)%SHELL_COUNT;
    if(*toShell == 0) *fromShell = (*fromShell + 1)%SHELL_COUNT;
    if(*fromShell == *toShell) return assignShell(fromShell, toShell);
}


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

    int p;
    int pid = getpid();
    int fpid;
    printf("Parent PID: %d\n", pid);
    int my_procnum = 0;
    int children_pid[TASK_COUNT];
    int fromShell = 0, toShell = 0;

    for(i = 0; i < TASK_COUNT && pid != 0; i++){
        assignShell(&fromShell, &toShell);
        // printf("%d: %d -> %d\n", i+1, fromShell, toShell);
        pid = fork();
        switch(pid){
            case -1:
                perror("Error with fork");
                exit(errno);

            case 0:
                my_procnum = i+1;
                int p = getpid();
                fprintf(stderr, "VCPU %d starting, pid %d\n", my_procnum, p);
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
            
            if(sem_try(shells+fromShell) == 1){ // successfully removed a shell
                sem_inc(shells + toShell);
            }
            else{
                sem_wait(shells + fromShell, my_procnum);
                sem_inc(shells + toShell);
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
                printf("VCPU %d: sleeps: %d wakes: %d\n", j+1, shells[i].sleep_count[j+1], shells[i].woken_count[j+1]);
            }
            printf("\n");
        }
    }

    return 0;

}