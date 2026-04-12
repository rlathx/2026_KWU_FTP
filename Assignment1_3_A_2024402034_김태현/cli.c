#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUF 1024

int main(int argc, char **argv) {
    // [error] 인자가 없으면 바로 종료
    if(argc < 2){
        exit(1);
    }

    // 1. 버퍼 생성 및 초기화
    char FTPcommand[MAX_BUF];
    memset(FTPcommand, 0, MAX_BUF);

    // 2. 명령어 변환 및 버퍼에 저장
    if (strcmp(argv[1], "ls") == 0) {
        strcpy(FTPcommand, "NLST");
        
        for(int i = 2; i < argc; i++){
            if(argv[i][0] == '-'){
                // [error - ls] 잘못된 옵션이 포함된 경우
                if((strcmp(argv[i], "-a") != 0) && (strcmp(argv[i], "-l") != 0) && (strcmp(argv[i], "-al") != 0)){
                    char *error_msg = "Error: invalid option\n";
                    write(STDERR_FILENO, error_msg, strlen(error_msg));
                    exit(1);
                }
            }

            // [error - ls] 와일드카드 '*' 체크
            if (strchr(argv[i], '*') != NULL) {
                char *error_msg = "Error: wildcard (*) is not supported\n";
                write(STDERR_FILENO, error_msg, strlen(error_msg));
                exit(1);
            }

            strcat(FTPcommand, " ");
            strcat(FTPcommand, argv[i]);
        }
    } 
    else if (strcmp(argv[1], "dir") == 0) {
        strcpy(FTPcommand, "LIST");

        for (int i = 2; i < argc; i++) {
            // [error - dir] 옵션이 포함된 경우
            if (argv[i][0] == '-') {
                char *error_msg = "Error: invalid option\n";
                write(STDERR_FILENO, error_msg, strlen(error_msg));
                exit(1);
            }

            strcat(FTPcommand, " ");
            strcat(FTPcommand, argv[i]);
        }
    } 
    else if (strcmp(argv[1], "pwd") == 0) {
        if (argc == 2) {
            // ./cli pwd
            strcpy(FTPcommand, "PWD");
        } 
        else if (argv[2][0] == '-') {
            // [error - pwd] 옵션이 포함된 경우
            char *error_msg = "Error: invalid option\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }
        else {
            // [error - pwd] 추가 인자가 있는 경우
            char *error_msg = "Error: argument is not required\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }
    } 
    else if (strcmp(argv[1], "cd") == 0) {
        // [error - cd] 추가 인자가 없는 경우
        if(argc < 3){
            char *error_msg = "Error: argument is required\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }

        // [error - cd] 옵션이 포함된 경우
        for (int i = 2; i < argc; i++) {
            if (argv[i][0] == '-') {
                char *error_msg = "Error: invalid option\n";
                write(STDERR_FILENO, error_msg, strlen(error_msg));
                exit(1);
            }
        }

        // [error - cd] 경로 인자가 하나 이상인 경우
        if (argc > 3) {
            char *err = "Error: only one argument is allowed\n";
            write(STDERR_FILENO, err, strlen(err));
            exit(1);
        }

        // 디렉토리명/경로 틀린 경우 예외 처리 -> 서버
        if (strcmp(argv[2], "..") == 0) {
            strcpy(FTPcommand, "CDUP");
        } else {
            strcpy(FTPcommand, "CWD ");
            strcat(FTPcommand, argv[2]);
        }
        
    } 
    else if (strcmp(argv[1], "mkdir") == 0) {
        // [error - mkdir] 추가 인자가 없는 경우
        if(argc < 3){
            char *error_msg = "Error: argument is required\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }
        
        // [error - mkdir] 옵션이 포함된 경우
        for (int i = 2; i < argc; i++) {
            if (argv[i][0] == '-') {
                char *error_msg = "Error: invalid option\n";
                write(STDERR_FILENO, error_msg, strlen(error_msg));
                exit(1);
            }
        }

        // 이미 존재하는 경우 예외 처리 -> 서버
        strcpy(FTPcommand, "MKD");
        // 복수 인자 처리
        for (int i = 2; i < argc; i++) {
            strcat(FTPcommand, " ");
            strcat(FTPcommand, argv[i]);
        }
    } 
    // 파일 삭제 명령어
    // sys call: unlink()
    else if (strcmp(argv[1], "delete") == 0) {
        // [error - delete] 추가 인자가 없는 경우
        if(argc < 3){
            char *error_msg = "Error: argument is required\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }

        // [error - delete] 옵션이 포함된 경우
        for (int i = 2; i < argc; i++) {
            if (argv[i][0] == '-') {
                char *error_msg = "Error: invalid option\n";
                write(STDERR_FILENO, error_msg, strlen(error_msg));
                exit(1);
            }
        }

        // 파일명이 없는 경우 예외 처리 -> 서버
        strcpy(FTPcommand, "DELE");
        // 복수 인자 처리
        for (int i = 2; i < argc; i++) { 
            strcat(FTPcommand, " ");
            strcat(FTPcommand, argv[i]);
        }
    } 
    // "비어 있는" 디렉토리 삭제 명령어
    // sys call: rmdir()
    else if (strcmp(argv[1], "rmdir") == 0) {
        // [error - rmdir] 추가 인자가 없는 경우
        if(argc < 3){
            char *error_msg = "Error: argument is required\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }

        // [error - rmdir] 옵션이 포함된 경우
        for (int i = 2; i < argc; i++) {
            if (argv[i][0] == '-') {
                char *error_msg = "Error: invalid option\n";
                write(STDERR_FILENO, error_msg, strlen(error_msg));
                exit(1);
            }
        }

        // 디렉토리가 없는 경우 예외 처리 -> 서버
        // 디렉토리가 비어 있지 않은 경우 예외 처리 -> 서버
        strcpy(FTPcommand, "RMD");
        // 복수 인자 처리
        for (int i = 2; i < argc; i++) { 
            strcat(FTPcommand, " ");
            strcat(FTPcommand, argv[i]);
        }
    } 
    else if (strcmp(argv[1], "rename") == 0) {
        // [error - rename] 인자가 딱 2개만 있어야함
        if (argc != 4) {
            char *error_msg = "Error: two arguments are required\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }

        // [error - rename] 옵션이 포함된 경우
        if (argv[2][0] == '-' || argv[3][0] == '-') {
            char *error_msg = "Error: invalid option\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }

        // 이미 존재하는 디렉토리/파일인 경우 예외 처리 -> 서버
        strcpy(FTPcommand, "RNFR ");
        strcat(FTPcommand, argv[2]);
        strcat(FTPcommand, " RNTO ");
        strcat(FTPcommand, argv[3]);
    } 
    else if (strcmp(argv[1], "get") == 0) {
        // [error - get] 추가 인자가 없는 경우
        if (argc < 3) {
            char *error_msg = "Error: argument is required\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }

        // [error - get] 옵션이 포함된 경우
        for (int i = 2; i < argc; i++) {
            if (argv[i][0] == '-') {
                char *error_msg = "Error: invalid option\n";
                write(STDERR_FILENO, error_msg, strlen(error_msg));
                exit(1);
            }
        }

        strcpy(FTPcommand, "RETR");
        // 복수 파일 다운로드 대응 (공백으로 구분하여 합침)
        for (int i = 2; i < argc; i++) {
            strcat(FTPcommand, " ");
            strcat(FTPcommand, argv[i]);
        }
    }
    else if (strcmp(argv[1], "put") == 0) {
        // [error - put] 추가 인자가 없는 경우
        if (argc < 3) {
            char *error_msg = "Error: argument is required\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }

        // [error - put] 옵션이 포함된 경우
        for (int i = 2; i < argc; i++) {
            if (argv[i][0] == '-') {
                char *error_msg = "Error: invalid option\n";
                write(STDERR_FILENO, error_msg, strlen(error_msg));
                exit(1);
            }
        }

        strcpy(FTPcommand, "STOR");
        // 복수 파일 업로드 대응
        for (int i = 2; i < argc; i++) {
            strcat(FTPcommand, " ");
            strcat(FTPcommand, argv[i]);
        }
    } 
    else if (strcmp(argv[1], "quit") == 0) {
        if (argc == 2) {
            // ./cli quit
            strcpy(FTPcommand, "QUIT");
        } 
        else if (argv[2][0] == '-') {
            // [error - quit] 옵션이 포함된 경우
            char *error_msg = "Error: invalid option\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }
        else {
            // [error - quit] 추가 인자가 있는 경우
            char *error_msg = "Error: argument is not required\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }
    }

    // 3. svr에 전달하기 위해 버퍼에 저장
    if (strlen(FTPcommand) > 0) {
        strcat(FTPcommand, "\n");
        write(STDOUT_FILENO, FTPcommand, strlen(FTPcommand));
    }

    exit(0);
}