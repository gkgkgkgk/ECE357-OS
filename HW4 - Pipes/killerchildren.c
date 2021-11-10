#include <stdio.h>
#include <unistd.h>
#include <sys/signal.h>

int l32 = 0;
int g32 = 0;

void handlerL32()
{
    l32++;
}

void handlerG32()
{
    g32++;
}

int main(int argc, char *argv[])
{
    int amountOfChildren = 100;
    int amountOfSignals = 1;

    if (argc > 1)
        amountOfChildren = atoi(argv[1]);

    if (argc > 2)
        amountOfSignals = atoi(argv[2]);

    int i = 0;
    int parentpid = getpid();

    signal(10, handlerL32);
    signal(35, handlerG32);

    int pids[amountOfChildren];

    for (i = 0; i < amountOfChildren; i++)
    {
        int pidTemp = 0;
        if ((pidTemp = fork()) == 0)
        {
            break;
        }
        pids[i] = pidTemp;
    }

    if (getpid() != parentpid)
    {
        int j = 0;
        for (j = 0; j < amountOfSignals; j++)
        {
            kill(parentpid, 35);
            kill(parentpid, 10);
        }
    }
    else
    {
        int j = 0;

        for (j = 0; j < amountOfChildren; j++)
        {
            int status = 0;
            int p = waitpid(pids[j], &status, 0);
        }

        printf("Amount of signals < 32: %d\n", l32);
        printf("Amount of signals > 32: %d g32\n", g32);
    }

    return 0;
}
