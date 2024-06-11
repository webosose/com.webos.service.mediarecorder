#include "log_info.h"
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

bool fileExists(const std::string &filename)
{
    std::ifstream file(filename);
    return file.good();
}

std::string getLastFile(const char *path, const char *ext)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(path); // set path to directory

    struct stat attr;
    time_t latest_tm = 0;

    std::string lastfile;

    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (strstr(dir->d_name, ext) == 0)
            {
                continue;
            }

            char name[256];
            sprintf(name, "%s%s", path, dir->d_name);

            stat(name, &attr);
            // printf(" %s : %s", name, ctime(&attr.st_mtime));

            if (attr.st_mtime > latest_tm)
            {
                // printf("%s : ", name);
                // printf("%s", ctime(&attr.st_mtime));
                latest_tm = attr.st_mtime;
                lastfile  = name;
            }
        }

        closedir(d);
    }

    return lastfile;
}

void deleteFile(const std::string &file)
{
    DEBUG_LOG("%s", file.c_str());

    if (fileExists(file))
    {
        int status;
        status = remove(file.c_str());
        if (status == 0)
            DEBUG_LOG("File Deleted Successfully!");
        else
            DEBUG_LOG("Error Occurred!");
    }
}

void deleteAll(const char *path, const char *ext)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(path); // set path to directory

    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            // printf("name : %s\n", dir->d_name);
            if (strstr(dir->d_name, ext) == 0)
            {
                continue;
            }

            char name[256];
            sprintf(name, "%s%s", path, dir->d_name);

            {
                int status;
                status = remove(name);
                if (status == 0)
                {
                    DEBUG_LOG("File Deleted Successfully : %s", name);
                }
                else
                    DEBUG_LOG("Error Occurred!");
            }
        }

        closedir(d);
    }
}
