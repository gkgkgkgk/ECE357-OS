#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/limits.h>
#include <errno.h>
#include <pwd.h>
#include <stdbool.h>
#include <sys/resource.h>
#include <sys/time.h>

int lastErr = -1;

void cd(char *directory)
{
    char *dir;
    if (strlen(directory) == 0 || directory[0] == '~' || directory == NULL)
    {
        dir = getenv("HOME");
    }
    else
    {
        // commented code supports relative paths for cd
        // char *curr_dir = getcwd(NULL, PATH_MAX);
        // dir = malloc(strlen(curr_dir) + strlen(directory) + 2);
        // strcpy(dir, curr_dir);
        // strcat(dir, "/");
        // strcat(dir, directory);
        // dir = strtok(dir, "\n");
        dir = strtok(directory, "\n");

        fprintf(stderr, "cd: %s\n", dir);
    }
    if (chdir(dir) != 0)
    {
        perror("Error Changing Directory: ");
    }
}

void pwd()
{
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
}

void exitShell(char *status)
{
    if (atoi(status) == 0)
    {
        fprintf(stderr, "Could not convert string to int\n");
    }
    else if (lastErr >= 0)
    {
        exit(lastErr);
    }
    else
    {
        exit(atoi(status));
    }
}

void redirectIO(char *io)
{
    char *redirect = strtok(io, " ");

    while (redirect != NULL)
    {
        int start = strchr(redirect, '>') != NULL ? (int)(strchr(redirect, '>') - redirect) : (int)(strchr(redirect, '<') - redirect);
        int nameStart = start + 1;
        int flags = O_TRUNC | O_CREAT | O_WRONLY;

        if (redirect[start] == redirect[start + 1])
        {
            flags = O_APPEND | O_CREAT | O_WRONLY;
            nameStart++;
        }

        char *filename = malloc(strlen(redirect) - start);
        strcpy(filename, redirect + nameStart);
        filename = strtok(filename, "\n");

        int fd = open(filename, flags, 0666);

        if (fd == -1)
        {
            fprintf(stderr, "Error opening file: %s", filename);
            exit(1);
        }

        if (start != 0)
        {
            dup2(fd, STDERR_FILENO);
        }
        else if (redirect[start] == '<')
        {
            flags = O_RDONLY;
            dup2(fd, STDIN_FILENO);
        }
        else
        {
            dup2(fd, STDOUT_FILENO);
        }

        close(fd);
        free(filename);

        redirect = strtok(NULL, " ");
    }
}

void executeCommand(char *command, char *args, char *io)
{
    int pid;
    struct timeval start, end;
    gettimeofday(&start, NULL);

    switch ((pid = fork()))
    {
    case -1:
        perror("Error forking process.");
        exit(1);
    case 0:
    {
        if (io != NULL && strlen(io) != 0)
        {
            redirectIO(io);
        }

        char *argv[strlen(args) + 1];
        char *token = strtok(args, " ");

        argv[0] = command;

        int i = 1;
        while (token != NULL)
        {
            argv[i] = token;
            token = strtok(NULL, " ");
            i++;
        }

        argv[i] = NULL;

        execvp(command, argv);
    }
    default:
    {
        int status;
        struct rusage ru;
        int pidC;

        if ((pidC = wait3(&status, 0, &ru)) == -1)
        {
            perror("Error waiting for child process.");
        }

        lastErr = status;

        if (status != 0)
        {
            if (WIFSIGNALED(status))
            {
                fprintf(stderr, "Process ID %d exited with signal %d.\n", pidC, WTERMSIG(status));
            }
            else
            {
                fprintf(stderr, "Process ID %d exited with signal %d.\n", pidC, WEXITSTATUS(status));
            }
        }
        else
        {
            fprintf(stderr, "Process ID %d exited normally.\n", pidC);
        }

        gettimeofday(&end, NULL);
        time_t realTime = end.tv_sec - start.tv_sec;
        time_t realTimeU = end.tv_usec - start.tv_usec;

        fprintf(stderr, "Real Time: %ld.%06u | User Time: %ld.%06u | System Time: %ld.%06u\n", realTime, realTimeU, ru.ru_utime.tv_sec, ru.ru_utime.tv_usec, ru.ru_stime.tv_sec, ru.ru_stime.tv_usec);
    }
    }
}

void handleArgs(char *line, int commandLength, int len, char *args, char *io)
{
    int lt = strchr(line, '<') != NULL ? (int)(strchr(line, '<') - line) : len;
    int gt = strchr(line, '>') != NULL ? (int)(strchr(line, '>') - line) : len;

    if (gt + lt == len * 2)
    {
        strncpy(args, line + commandLength + 1, len - commandLength);
        args[len - commandLength] = '\0';

        int i;
        for (i = 0; i < strlen(args); i++)
        {
            if (args[i] == '\n')
            {
                args[i] = '\0';
            }
        }
    }
    else
    {
        bool freeIO = true;
        bool freeArgs = true;

        char *w = strtok(line + commandLength + 1, " ");
        while (w != NULL)
        {
            if (strchr(w, '<') != NULL)
            {
                strcat(io, w);
                strcat(io, " ");
                freeIO = false;
            }
            else if (strchr(w, '>') != NULL)
            {
                strcat(io, w);
                strcat(io, " ");
                freeIO = false;
            }
            else
            {
                strcat(args, w);
                strcat(args, " ");
                freeArgs = false;
            }

            w = strtok(NULL, " ");
        }

        if (freeIO)
        {
            free(io);
        }

        if (freeArgs)
        {
            free(args);
        }
    }
}

void executeLine(char *line)
{
    if (line[0] == '#' || line[0] == '\0') // ignore comment or empty line
    {
        return;
    }

    int len = strlen(line);

    int commandLength = strchr(line, ' ') != NULL ? (int)(strchr(line, ' ') - line) : len - 1;
    char *command = malloc(commandLength + 1);
    strncpy(command, line, commandLength);
    command[commandLength] = '\0';

    char *args = malloc(len - commandLength + 1); // this is a very small waste of space
    char *io = malloc(len - commandLength + 1);

    handleArgs(line, commandLength, len, args, io);

    if (strcmp(command, "cd") == 0)
    {
        cd(args);
    }
    else if (strcmp(command, "pwd") == 0)
    {
        pwd();
    }
    else if (strcmp(command, "exit") == 0)
    {
        exitShell(args);
    }
    else
    {
        executeCommand(command, args, io);
    }
}

void invokeShell(FILE *fp)
{
    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, fp) >= 0)
    {
        executeLine(line);
    }
}

int main(int argc, char *argv[])
{
    FILE *fp = argc > 1 ? fopen(argv[1], "r") : stdin;

    if (fp == NULL)
    {
        printf("File not found\n");
        return 1;
    }

    invokeShell(fp);

    return 0;
}[
