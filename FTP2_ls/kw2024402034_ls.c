#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    DIR* dp; 
    struct dirent* dirp;
    char* path;

    // error 1. 인자 개수 초과
    if (argc > 2) {
        printf("only one directory path can be processed\n");

        return 0;
    }

    // 대상 디렉토리 설정 (인자가 없으면 현재 디렉토리 ".")
    if (argc == 1) {
        path = ".";
    } else {
        path = argv[1];
    }

    // 디렉토리 열기 
    dp = opendir(path);

    // error 2. 존재하지 않는 dir
    // error 3. 접근 권한 없음
    if (dp == NULL) {
        if (errno == ENOENT) { // 파일이나 디렉토리가 없는 경우
            printf("%s: cannot access '%s': No such directory\n", argv[0], path);
        } 
        else if (errno == EACCES) { // 권한이 거부된 경우 (Permission denied)
            printf("%s: cannot access '%s': Access denied\n", argv[0], path);
        } 

        closedir(dp);
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