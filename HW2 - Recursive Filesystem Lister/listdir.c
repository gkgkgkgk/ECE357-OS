#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

int blockSize = 1;

// generate string to list inode number, number of blocks, file mode, n_link, owner, file group, file size, last modified date, file name
void convertFileMode(int mode, char *fileMode)
{
    char *type = "";

    if (S_ISDIR(mode))
    {
        type = "d";
    }
    else if (S_ISCHR(mode))
    {
        type = "c";
    }
    else if (S_ISBLK(mode))
    {
        type = "b";
    }
    else if (S_ISFIFO(mode))
    {
        type = "p";
    }
    else if (S_ISLNK(mode))
    {
        type = "l";
    }
    else if (S_ISSOCK(mode))
    {
        type = "s";
    }
    else if (S_ISREG(mode))
    {
        type = "-";
    }
    else
    {
        type = "?";
        fprintf(stderr, "Error! File type is unkown.\n");
    }

    strcpy(fileMode, type);
    strcat(fileMode, (mode & S_IRUSR) ? "r" : "-");
    strcat(fileMode, (mode & S_IWUSR) ? "w" : "-");
    strcat(fileMode, (mode & S_IXUSR) ? "x" : "-");
    strcat(fileMode, (mode & S_IRGRP) ? "r" : "-");
    strcat(fileMode, (mode & S_IWGRP) ? "w" : "-");
    strcat(fileMode, (mode & S_IXGRP) ? "x" : "-");
    strcat(fileMode, (mode & S_IROTH) ? "r" : "-");
    strcat(fileMode, (mode & S_IWOTH) ? "w" : "-");
    strcat(fileMode, (mode & S_IXOTH) ? "x" : "-");
}

void printFile(struct stat *sb, char *path)
{
    unsigned int fileModeInt = sb->st_mode;
    char *owner;
    char *fileGroup;
    unsigned long fileSize = sb->st_size;

    char fileMode[10];
    convertFileMode(fileModeInt, fileMode);

    struct passwd *pw = getpwuid(sb->st_uid);
    if (pw)
    {
        owner = pw->pw_name;
    }
    else
    {
        owner = "?";
        fprintf(stderr, "Error! File owner could not be found.\n");
    }

    struct group *gr = getgrgid(sb->st_gid);
    if (gr)
    {
        fileGroup = gr->gr_name;
    }
    else
    {
        fileGroup = "?";
        fprintf(stderr, "Error! Could File group could not be found.\n");
    }

    char *lastModified = malloc(80);
    strftime(lastModified, 80, "%b %e %H:%M", localtime(&sb->st_mtime));

    printf("    %lu %6lu %s %3lu %-8s %-8s %8lu %s %s\n", sb->st_ino, sb->st_blocks / blockSize, fileMode, sb->st_nlink, owner, fileGroup, fileSize, lastModified, path);
}

void listdir(char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        fprintf(stderr, "Error! Could not open directory %s.\n", path);
        return;
    }
    struct dirent *entry;
    struct stat sb;
    struct stat db;

    lstat(path, &db);
    printFile(&db, path);
    char newPath[4096];

    if (dir != NULL)
    {
        while ((entry = readdir(dir)) != NULL)
        {
            if (!(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0))
            {

                strcpy(newPath, path);
                strcat(newPath, "/");
                strcat(newPath, entry->d_name);
                lstat(newPath, &sb);

                if (entry->d_type == DT_DIR)
                {
                    listdir(newPath);
                }
                else
                {
                    printFile(&sb, newPath);
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    char *e = getenv("POSIXLY_CORRECT");
    blockSize = (e != NULL ? 1 : 2);
    listdir((argc == 1) ? "." : argv[1]);
}

/* references:
https://www.gnu.org/software/findutils/manual/html_node/find_html/Print-File-Information.html
https://www.gnu.org/software/libc/manual/html_node/Testing-File-Type.html
*/