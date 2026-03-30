#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
// File Name : kw2024402034_ls.c
// Date      : 2026/03/29
// OS        : Ubuntu 20.04.6 LTS 64bits
// Author    : Kim Tae Hyeon
// Student ID: 2024402034
// -------------------------------------------------------------------- //
// Title     : System Programming Assignment #1-2 ( ftp server )   
// Description : ...
//
//
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
    DIR* dp; 
    struct dirent* dirp;
    char* path;

    // error 1. 인자 개수 초과
    if (argc > 2) {
        printf("only one directory path can be processed\n");

        return 0;
    }

    // set target directory (if not an argument, target dir == present dir(./))
    if (argc == 1) {
        path = ".";
    } else {
        path = argv[1];
    }

    // open directory
    dp = opendir(path);
    
    // error control
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

    // 디렉토리 순회 및 파일 이름 출력 (File names only)
    while ((dirp = readdir(dp)) != NULL) {
        printf("%s\n", dirp->d_name);
    }

    // 5. 디렉토리 닫기
    closedir(dp);

    return 0;
}