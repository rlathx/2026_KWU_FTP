#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    DIR *dp;
    struct dirent *dirp;

    // 디렉토리 열기 (인자 없으면 현재 디렉토리)
    if (argc == 1) {
        dp = opendir(".");
    } else {
        dp = opendir(argv[1]);
    }

    // 디렉토리 순회
    while ((dirp = readdir(dp)) != NULL) {
        printf("%s\n", dirp->d_name);
    }

    // 디렉토리 닫기
    closedir(dp);

    return 0;
}