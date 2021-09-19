#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define BUF_SIZE 4096

void terminate(char *fileName, char *call)
{
    fprintf(stderr, "Error! Program Terminated. File: %s | Call: %s\n", fileName, call);
    exit(-1);
}

bool checkForBinary(char *buf, int length, char *inputFile)
{
    for (int i = 0; i < length; i++)
    {
        if (!(isprint(buf[i]) || isspace(buf[i])))
        {
            fprintf(stderr, "Warning! %s is a binary file.\n", inputFile);
            return true;
        }
    }

    return false;
}

void partialWrite(char *buf, int r, int w, int fd, int *writeCount)
{
    char unwrittenBuf[r - w];

    for (int i = w; i < r; i++)
    {
        unwrittenBuf[i - w] = buf[i];
    }

    write(fd, unwrittenBuf, r - w);

    writeCount++;
}

void concat(int *outputFile, char *inputFile)
{
    char buf[BUF_SIZE];
    int fd = strcmp(inputFile, "-") == 0 ? STDIN_FILENO : open(inputFile, O_RDONLY, 0666);

    if (fd < 0)
        terminate(inputFile, "open");

    int r = read(fd, buf, BUF_SIZE);

    if (r < 0)
        terminate(inputFile, "read");

    bool isBinary = false;
    int writeCount = 0;
    int readCount = 1;
    int bytesWritten = 0;

    while (r > 0)
    {
        isBinary = isBinary ? true : checkForBinary(buf, r, inputFile);
        int w = write(*outputFile, buf, r);
        if (w < 0)
            terminate("output", "write");

        bytesWritten += w;
        writeCount++;

        if (w != r)
        {
            partialWrite(buf, r, w, *outputFile, &writeCount);
        }

        r = read(fd, buf, BUF_SIZE);
        if (r < 0)
            terminate(inputFile, "read");

        readCount++;
    }

    int c = close(fd);

    if (c < 0)
        terminate(inputFile, "close");

    fprintf(stderr, "Finished concat for file: %s | Bytes transferred: %d | Read count: %d | Write count: %d\n", strcmp(inputFile, "-") == 0 ? "stdin" : inputFile, bytesWritten, readCount, writeCount);

    if (isBinary)
        fprintf(stderr, "Warning! File %s is a binary file!\n", inputFile);
}

int main(int argc, char *argv[])
{
    int c;
    int outputFile = STDOUT_FILENO;

    while ((c = getopt(argc, argv, "o:")) != -1)
    {
        if (c == 'o')
        {
            outputFile = open(optarg, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (outputFile < 0)
                terminate("output", "open");
        }
    }

    if (optind >= argc)
    {
        concat(&outputFile, "-");
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            concat(&outputFile, argv[i]);
        }
    }

    return 0;
}