#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// File Name : kw2024402034_ls.c                                                                  //
// Date      : 2026/03/29                                                                         //
// OS        : Ubuntu 20.04.6 LTS 64bits                                                          //
// Author    : Kim Tae Hyeon                                                                      //
// Student ID: 2024402034                                                                         //
// ---------------------------------------------------------------------------------------------- //
// Title     : System Programming Assignment #1-2 ( ftp server )                                  //
// Description :                                                                                  //
// This program lists the files and directories within a specified path.                          //
// If no argument is provided, it defaults to the current directory (.).                          //
// It handles exceptions for excessive arguments and uses opendir() to access the target path,    //
// with specific error handling for non-existent paths (ENOENT), non-directory files (ENOTDIR),   //
// and permission denials (EACCES).                                                               //
// It then sequentially prints the names of all entries using readdir()                           //
// before releasing system resources with closedir().                                             //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
    DIR* dp; 
    struct dirent* dirp;
    char* path;

    // error 1. 인자 개수 초과
    if (argc > 2) {
        printf("only one directory path can be processed\n");

        return 0;
    }

    // set the target directory
    if (argc == 1) {
        path = ".";
    } else {
        path = argv[1];
    }

    // open directory
    dp = opendir(path);
    
    // error handling
    if (dp == NULL) {

        char *programName = strrchr(argv[0], '/');
        programName++;

        if ((errno == ENOENT) || (errno == ENOTDIR)) { 
            // error 2. 존재하지 않는 dir
            // error 3. ENOTDIR - 존재하지만 dir이 아닌 file type
            printf("%s: cannot access '%s': No such directory\n", programName, path);
        } 
        else if (errno == EACCES) { 
            // error 4. EACCES - 존재하지만 접근 권한 없음
            printf("%s: cannot access '%s': Access denied\n", programName, path);
        } 

        return 0;
    }

    // read directory entry (File names only)
    while ((dirp = readdir(dp)) != NULL) {
        printf("%s\n", dirp->d_name);
    }

    // close directory
    closedir(dp);

    return 0;
}