#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUF 1024

int main(int argc, char **argv){
    // 1. 버퍼 생성 및 초기화
    char FTPcommand[MAX_BUF];
    memset(FTPcommand, 0, MAX_BUF);

    // 2. cli로부터 데이터 수신
    ssize_t receivedBytesNum = read(STDIN_FILENO, FTPcommand, MAX_BUF);

    // [error] 수신할 데이터 없음
    if(receivedBytesNum <= 0) exit(1);

    // 3. 명령어 해석 및 실행
    if (strncmp(FTPcommand, "NLST", 4) == 0) {
        // [NLST/LIST 처리] 목록 출력 로직 예정
        write(STDOUT_FILENO, "Processing LS/DIR...\n", 21);
    }
    else if (strncmp(FTPcommand, "LIST", 4) == 0){

    }
    else if (strncmp(FTPcommand, "PWD", 3) == 0) {
        char currentWD[MAX_BUF];

        // [error - PWD] return NULL 대비
        if(getcwd(currentWD, sizeof(currentWD)) == NULL){
            char* error_msg = "Error: directory not found\n";
            write(STDOUT_FILENO, error_msg, strlen(error_msg));

            exit(1);
        }

        write(STDOUT_FILENO, "\'", 2);
        write(STDOUT_FILENO, currentWD, strlen(currentWD));
        write(STDOUT_FILENO, "\'", 2);
        write(STDOUT_FILENO, " is current directory", 22);
    }
    else if (strncmp(FTPcommand, "CWD", 3) == 0) {
        strtok(FTPcommand, " ");
        char *targetWD = strtok(NULL, " ");

        // [error - CWD] 경로명 틀림
        if(targetWD == NULL || chdir(targetWD) == -1){
            char* error_msg = "Error: directory not found\n";
            write(STDOUT_FILENO, error_msg, strlen(error_msg));

            exit(1);
        }

        write(STDOUT_FILENO, "\'", 2);
        write(STDOUT_FILENO, targetWD, strlen(targetWD));
        write(STDOUT_FILENO, "\'", 2);
        write(STDOUT_FILENO, " is current directory", 22);
    }
    else if(strncmp(FTPcommand, "CDUP", 4) == 0){
        if(chdir("..") == -1){
            char* error_msg = "Error: directory not found\n";
            write(STDOUT_FILENO, error_msg, strlen(error_msg));

            exit(1);
        }

        char currentWD[MAX_BUF];

        // [error - CDUP] return NULL 대비
        if(getcwd(currentWD, sizeof(currentWD)) == NULL){
            char* error_msg = "Error: directory not found\n";
            write(STDOUT_FILENO, error_msg, strlen(error_msg));

            exit(1);
        }

        write(STDOUT_FILENO, "\'", 2);
        write(STDOUT_FILENO, currentWD, strlen(currentWD));
        write(STDOUT_FILENO, "\'", 2);
        write(STDOUT_FILENO, " is current directory", 22);
    }
    else if (strncmp(FTPcommand, "MKD", 3) == 0) {
        char* dirName = strtok(FTPcommand, " ");

        while((dirName = strtok(NULL, " ")) != NULL){
            if(mkdir(dirName, 0775) == -1){
                write(STDOUT_FILENO, "Error: cannot create directory \'", 33);
                write(STDOUT_FILENO, dirName, strlen(dirName));
                write(STDOUT_FILENO, "\': File exists\n", 16);
            }
        }
    }
    else if (strncmp(FTPcommand, "DELE", 4) == 0) {
        char* dirName = strtok(FTPcommand, " ");

        while((dirName = strtok(NULL, " ")) != NULL){
            if(unlink(dirName) == -1){
                write(STDOUT_FILENO, "Error: failed to delete file\n", 45);
            }
        }
    }
    else if (strncmp(FTPcommand, "RMD", 3) == 0) {
        char* dirName = strtok(FTPcommand, " ");

        while((dirName = strtok(NULL, " ")) != NULL){
            if(rmdir(dirName) == -1){
                write(STDOUT_FILENO, "Error: failed to remove \'", 26);
                write(STDOUT_FILENO, dirName, strlen(dirName));
                write(STDOUT_FILENO, "\'\n", 3);
            }
        }
    }
    else if (strncmp(FTPcommand, "RNFR", 4) == 0) {
        strtok(FTPcommand, " ");
        char* oldname = strtok(NULL, " ");
        strtok(NULL, " ");
        char* newname = strtok(NULL, " ");

        if (oldname == NULL || newname == NULL) {
            char* error_msg = "Error: argument is required\n";
            write(STDOUT_FILENO, error_msg, strlen(error_msg));
            exit(1);
        }

        // [error - RNFR&RNTO] 이미 있는 이름으로 변경 요청
        if (rename(oldname, newname) == -1){
            write(STDOUT_FILENO, "Error: name to change already exists\n", 38);
            exit(1);
        }
    }
    else if (strncmp(FTPcommand, "RETR", 4) == 0) {
    }
    else if (strncmp(FTPcommand, "STOR", 4) == 0) {
        // [STOR 처리] 파일 수신(업로드) 로직 예정
        write(STDOUT_FILENO, "Processing PUT...\n", 18);
    }
    else if (strncmp(FTPcommand, "QUIT", 4) == 0) {
        write(STDOUT_FILENO, "Quit success\n", 14);
    }

    exit(0);
}