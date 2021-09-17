#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define BUF_SIZE 4096

// TODO:
// Add error handling and test it
// See if printing to stdout is good because the messages will be in between files.

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

    writeCount++;
}

void concat(int *outputFile, char *inputFile)
{
    char buf[BUF_SIZE];
    int fd = strcmp(inputFile, "-") == 0 ? STDIN_FILENO : open(inputFile, O_RDONLY, 0666);
    int r = read(fd, buf, BUF_SIZE);
    bool isBinary = false;
    int writeCount = 0;
    int readCount = 1;

    while (r > 0)
    {
        isBinary = isBinary ? true : checkForBinary(buf, r, inputFile);
        int w = write(*outputFile, buf, r);

        writeCount++;

        if (w != r)
        {
            partialWrite(buf, r, w, *outputFile, &writeCount);
        }

        r = read(fd, buf, BUF_SIZE);
        readCount++;
    }

    //fprintf(stderr, "%s", "hi"); // TODO correct print message here
}

int main(int argc, char *argv[])
{
    int c;
    bool writeOutput = false;

    while ((c = getopt(argc, argv, "o::")) != -1)
    {
        if (c == 'o')
            writeOutput = true;
    }

    int outputFile = writeOutput ? open("file3", O_WRONLY | O_CREAT | O_TRUNC, 0666) : STDOUT_FILENO;

    for (int i = optind; i < argc; i++)
    {
        concat(&outputFile, argv[i]);
    }

    return 0;
}