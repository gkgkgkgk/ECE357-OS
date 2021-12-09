#include "sem.h"
#include "spinlock.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void handler(int signum)
{
    return;
}

void sem_init(struct sem *s, int count)
{
    s->count = count;
    s->spinlock = 0;
    int i;
    memset(s->sleeping, 0, N_PROC);
    memset(s->sleep_count, 0, N_PROC);
    memset(s->woken_count, 0, N_PROC);
}

int sem_try(struct sem *s)
{
    int val = 0;
    spin_lock(&s->spinlock);

    if (s->count > 0)
    {
        s->count--;
        val = 1;
    }

    spin_unlock(&s->spinlock);
    return val;
}

void sem_wait(struct sem *s, int my_procnum)
{
    while (1)
    {
        spin_lock(&s->spinlock);
        if (s->count > 0)
        {
            s->count--;
            spin_unlock(&s->spinlock);
            return;
        }

        s->sleeping[my_procnum] = getpid();

        sigset_t newmask, oldmask;
        sigfillset(&newmask);
        signal(SIGUSR1, handler);
        sigprocmask(SIG_BLOCK, &newmask, &oldmask);
        spin_unlock(&s->spinlock);
        s->sleep_count[my_procnum]++;
        sigsuspend(&oldmask);

        sigprocmask(SIG_UNBLOCK, &newmask, &oldmask);
        signal(SIGUSR1, SIG_DFL);
        s->sleeping[my_procnum] = 0;
        s->handle_count[my_procnum]++;
    }
}

void sem_inc(struct sem *s)
{
    spin_lock(&s->spinlock);
    s->count++;
    if (s->count > 0)
    {
        int i;
        for (i = 0; i < N_PROC; i++)
        {
            if (s->sleeping[i] != 0)
            {
                s->woken_count[i]++;
                kill(s->sleeping[i], SIGUSR1);
                s->sleeping[i] = 0;
            }
        }
    }
    spin_unlock(&s->spinlock);
    return;
}