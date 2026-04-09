#include <string.h>
#include <unistd.h>

#define MAX_BUF 1024

int main(int argc, char **argv) {
    // [error] 인자가 없으면 바로 종료
    if(argc < 2){
        return 0;
    }

    // 1. 버퍼 생성 및 초기화
    char FTPcommand[MAX_BUF];
    memset(FTPcommand, 0, MAX_BUF);

    // 2. 명령어 변환 및 버퍼에 저장
    if (strcmp(argv[1], "ls") == 0) {
        strcpy(FTPcommand, "NLST");
        
    } 
    else if (strcmp(argv[1], "dir") == 0) {
        strcpy(FTPcommand, "LIST");
    } 
    else if (strcmp(argv[1], "pwd") == 0) {
        strcpy(FTPcommand, "PWD");
    } 
    else if (strcmp(argv[1], "cd") == 0) {
        if (argc < 2){
            strcpy(FTPcommand, "CWD");
        }
        else if (argc == 2) {
            strcpy(FTPcommand, "CDUP");
        }
        else {
            // error
        }
        
    } 
    else if (strcmp(argv[1], "mkdir") == 0) {
        strcpy(FTPcommand, "MKD");
        
    } 
    else if (strcmp(argv[1], "delete") == 0) {
        strcpy(FTPcommand, "DELE");
    } 
    else if (strcmp(argv[1], "rmdir") == 0) {
        strcpy(FTPcommand, "RMD");
    } 
    else if (strcmp(argv[1], "rename") == 0) {
        // 이름 바꾸는 경우
        // 위치 바꾸는 경우
    } 
    else if (strcmp(argv[1], "get") == 0) {
        strcpy(FTPcommand, "RETR");
    }
    else if (strcmp(argv[1], "put") == 0) {
        strcpy(FTPcommand, "STOR");
    } 
    else if (strcmp(argv[1], "quit") == 0) {
        strcpy(FTPcommand, "QUIT");
    }

    // 3. svr에 전달하기 위해 버퍼에 저장
    if (strlen(FTPcommand) > 0) {
        write(STDOUT_FILENO, FTPcommand, strlen(FTPcommand));
    }

    return 0;
}