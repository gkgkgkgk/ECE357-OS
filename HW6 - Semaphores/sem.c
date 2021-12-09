#include "sem.h"
#include "spinlock.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void handler(int signum){
    // increment something
    return;
}

void sem_init(struct sem* s, int count){
    s->count = count;
    s->spinlock = 0;
    int i;
    memset(s->sleeping, 0, N_PROC);
    memset(s->sleep_count, 0, N_PROC);
    memset(s->woken_count, 0, N_PROC);
}


int sem_try(struct sem* s){
    int val = 0;
    spin_lock(&s->spinlock);

    if(s->count > 0 && s->spinlock == 0){
        s->count--;
        val = 1;
    }

    spin_unlock(&s->spinlock);
    return val;
}


void sem_wait(struct sem *s, int my_procnum){
    spin_lock(&s->spinlock);
    if(s->count > 0){
        s->count--;
        spin_unlock(&s->spinlock);
        return;
    }

    s->sleeping[my_procnum] = getpid();
    
    s->sleep_count[my_procnum]++;
    spin_unlock(&s->spinlock);

    sigset_t newmask,oldmask;
    sigfillset(&newmask);
    signal(SIGUSR1, handler);
    sigprocmask(SIG_BLOCK,&newmask,&oldmask);
    sigsuspend(&oldmask);

    sigprocmask(SIG_UNBLOCK,&newmask,&oldmask);
    signal(SIGUSR1, SIG_DFL);
    s->sleeping[my_procnum] = 0;
    s->woken_count[my_procnum]++;

    return sem_wait(s, my_procnum);
}


void sem_inc(struct sem *s){
    spin_lock(&s->spinlock);
    s->count++;
    if(s->count > 0){
        int i;
        for(i = 0; i < N_PROC; i++){
            if(s->sleeping[i] != 0){
                kill(s->sleeping[i], SIGUSR1);
                s->sleeping[i] = 0;
            }
        }
    }
    spin_unlock(&s->spinlock);
    return;
}