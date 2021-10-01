#include <stdio.h>
void readdir(char *path)
{
    //print path
    printf("%s\n", path);
}

int main(int argc, char *argv[])
{
    readdir((argc == 1) ? "." : argv[1]);
}
