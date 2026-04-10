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
        // [PWD 처리] 현재 경로 출력 로직 예정
        write(STDOUT_FILENO, "Processing PWD...\n", 18);
    }
    else if (strncmp(FTPcommand, "CWD", 3) == 0) {
        // [CWD/CDUP 처리] 디렉토리 이동 로직 예정
        write(STDOUT_FILENO, "Processing CD...\n", 17);
    }
    else if(strncmp(FTPcommand, "CDUP", 4) == 0){

    }
    else if (strncmp(FTPcommand, "MKD", 3) == 0) {
        // [MKD 처리] 디렉토리 생성 로직 예정
        write(STDOUT_FILENO, "Processing MKDIR...\n", 20);
    }
    else if (strncmp(FTPcommand, "DELE", 4) == 0) {
        // [DELE 처리] 파일 삭제 로직 예정
        write(STDOUT_FILENO, "Processing DELETE...\n", 21);
    }
    else if (strncmp(FTPcommand, "RMD", 3) == 0) {
        // [RMD 처리] 디렉토리 삭제 로직 예정
        write(STDOUT_FILENO, "Processing RMDIR...\n", 20);
    }
    else if (strncmp(FTPcommand, "RNFR", 4) == 0) {
        // [RNFR/RNTO 처리] 이름 변경/위치 이동 로직 예정
        write(STDOUT_FILENO, "Processing RENAME...\n", 21);
    }
    else if (strncmp(FTPcommand, "RETR", 4) == 0) {
        // [RETR 처리] 파일 전송(다운로드) 로직 예정
        write(STDOUT_FILENO, "Processing GET...\n", 18);
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