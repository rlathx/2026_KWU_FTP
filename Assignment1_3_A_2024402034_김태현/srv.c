#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include <pwd.h>
#include <grp.h>
#include <time.h>

#define MAX_BUF 1024

void itoc(long n);
void writePermissions(mode_t mode);
void myls(char *option, char *path);

////////////////////////////////////////////////////////////////////////////////////////////////////
// File Name : svr.c                                                                              //
// Date      : 2026/04/15                                                                         //
// OS        : Ubuntu 20.04.6 LTS 64bits                                                          //
// Author    : Kim Tae Hyeon                                                                      //
// Student ID: 2024402034                                                                         //
// ---------------------------------------------------------------------------------------------- //
// Title     : System Programming Assignment #1-3 ( ftp server )                                  //
// Description :                                                                                  //
// This program interprets FTP commands received from the client,                                 //
// performs the corresponding operations, and outputs the results.                                //
//                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////

int main(){
    // 1. 버퍼 생성 및 초기화
    char FTPcommand[MAX_BUF];
    memset(FTPcommand, 0, MAX_BUF);

    // 2. cli로부터 데이터 수신
    ssize_t receivedBytesNum = read(STDIN_FILENO, FTPcommand, MAX_BUF);
    FTPcommand[strcspn(FTPcommand, "\n")] = 0; // 개행 문자 제거

    // [error] 수신할 데이터 없음
    if(receivedBytesNum <= 0) exit(1);

    // 3. 명령어 해석 및 실행
    if (strncmp(FTPcommand, "NLST", 4) == 0) {
        char *option = NULL;
        char *path = ".";
        
        // 명령어 "NLST" 이후의 인자 파싱
        char *token = strtok(FTPcommand, " "); // "NLST"
        while ((token = strtok(NULL, " ")) != NULL) {
            if (token[0] == '-') option = token;
            else path = token;
        }

        write(STDOUT_FILENO, "NLST ", 6);
        if(option != NULL) write(STDOUT_FILENO, option, strlen(option));
        write(STDOUT_FILENO, "\n", 2);

        myls(option, path);
    }
    else if (strncmp(FTPcommand, "LIST", 4) == 0){
        char *option = "-al"; // dir은 ls -al과 동일한 로직 수행
        char *path = ".";
        
        // path 추출
        strtok(FTPcommand, " ");
        char *token = strtok(NULL, " ");
        
        if (token != NULL) {
            path = token;
        }

        write(STDOUT_FILENO, "LIST\n", 6);

        myls(option, path);
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
        write(STDOUT_FILENO, " is current directory\n", 23);
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

        char absolutePath[MAX_BUF];
        getcwd(absolutePath, sizeof(absolutePath));

        write(STDOUT_FILENO, "CWD ", 5);
        write(STDOUT_FILENO, targetWD, strlen(targetWD));
        write(STDOUT_FILENO, "\n", 2);

        write(STDOUT_FILENO, "\'", 2);
        write(STDOUT_FILENO, absolutePath, strlen(absolutePath));
        write(STDOUT_FILENO, "\'", 2);
        write(STDOUT_FILENO, " is current directory\n", 23);
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

        write(STDOUT_FILENO, "CDUP\n", 6);

        write(STDOUT_FILENO, "\'", 2);
        write(STDOUT_FILENO, currentWD, strlen(currentWD));
        write(STDOUT_FILENO, "\'", 2);
        write(STDOUT_FILENO, " is current directory\n", 23);
    }
    else if (strncmp(FTPcommand, "MKD", 3) == 0) {
        strtok(FTPcommand, " ");
        char* dirName;

        while((dirName = strtok(NULL, " ")) != NULL){
            if(mkdir(dirName, 0775) == -1){
                write(STDOUT_FILENO, "Error: cannot create directory \'", 33);
                write(STDOUT_FILENO, dirName, strlen(dirName));
                write(STDOUT_FILENO, "\': File exists\n", 16);

                continue;
            }

            write(STDOUT_FILENO, "MKD ", 5);
            write(STDOUT_FILENO, dirName, strlen(dirName));
            write(STDOUT_FILENO, "\n", 2);
        }
    }
    else if (strncmp(FTPcommand, "DELE", 4) == 0) {
        strtok(FTPcommand, " ");
        char* dirName;

        while((dirName = strtok(NULL, " ")) != NULL){
            if(unlink(dirName) == -1){
                write(STDOUT_FILENO, "Error: failed to stat the file or directory\n", 45);
                continue;
            }
            write(STDOUT_FILENO, "DELE ", 6);
            write(STDOUT_FILENO, dirName, strlen(dirName));
            write(STDOUT_FILENO, "\n", 2);
        }
    }
    else if (strncmp(FTPcommand, "RMD", 3) == 0) {
        strtok(FTPcommand, " ");
        char* dirName; 

        while((dirName = strtok(NULL, " ")) != NULL){
            if(rmdir(dirName) == -1){
                write(STDOUT_FILENO, "Error: failed to remove \'", 26);
                write(STDOUT_FILENO, dirName, strlen(dirName));
                write(STDOUT_FILENO, "\'\n", 3);

                continue;
            }
            write(STDOUT_FILENO, "RMD ", 5);
            write(STDOUT_FILENO, dirName, strlen(dirName));
            write(STDOUT_FILENO, "\n", 2);
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

        if(access(newname, F_OK) == 0){
            write(STDOUT_FILENO, "Error: name to change already exists\n", 38);
            exit(1);
        }

        // [error - RNFR&RNTO]
        // 변경하고자 하는 파일이 존재하지 않음
        // 권한이 없음
        // 다른 파일 시스템으로 이동 시도
        if (rename(oldname, newname) == -1){
            write(STDOUT_FILENO, "Error: failed to rename\n", 25);
        }

        write(STDOUT_FILENO, "RNFR ", 6);
        write(STDOUT_FILENO, oldname, strlen(oldname));
        write(STDOUT_FILENO, "\nRNTO ", 7);
        write(STDOUT_FILENO, newname, strlen(newname));
        write(STDOUT_FILENO, "\n", 2);
    }
    else if (strncmp(FTPcommand, "RETR", 4) == 0) { // get
        // 1. 파일명 추출
        strtok(FTPcommand, " ");
        char* filename = strtok(NULL, " ");

        // 2. 경로 설정
        char pathFR[MAX_BUF]; // from server_root
        strcpy(pathFR, "server_root/");
        strcat(pathFR, filename);

        char pathTO[MAX_BUF]; // to client_root
        strcpy(pathTO, "client_root/");
        strcat(pathTO, filename);

        // 3. 파일 존재 여부 확인
        int fdFR = open(pathFR, O_RDONLY);
        if(fdFR == -1){
            write(STDERR_FILENO, "Error: \'", 9);
            write(STDERR_FILENO, filename, strlen(filename));
            write(STDERR_FILENO, "\' does not exist in server_root\n", 33);

            exit(1);
        }

        int fdTO = open(pathTO, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        char buf[MAX_BUF];
        ssize_t nread, total_bytes = 0;

        while ((nread = read(fdFR, buf, MAX_BUF)) > 0) {
            write(fdTO, buf, nread);
            total_bytes += nread;
        }
        
        write(STDOUT_FILENO, "RETR ", 6);
        write(STDOUT_FILENO, filename, strlen(filename));
        write(STDOUT_FILENO, "\nOK: ", 5);
        itoc(total_bytes);
        write(STDOUT_FILENO, " bytes copied to client_root\n", 29);

        close(fdFR);
        close(fdTO);
    }
    else if (strncmp(FTPcommand, "STOR", 4) == 0) { // put
        // 1. 파일명 추출
        strtok(FTPcommand, " ");
        char* filename = strtok(NULL, " ");

        // 2. 경로 설정
        char pathFR[MAX_BUF]; // from client_root
        strcpy(pathFR, "client_root/");
        strcat(pathFR, filename);

        char pathTO[MAX_BUF]; // to server_root
        strcpy(pathTO, "server_root/");
        strcat(pathTO, filename);

        // 파일 존재 여부 확인은 cli에서 함
        int fdFR = open(pathFR, O_RDONLY);
        int fdTO = open(pathTO, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        char buf[MAX_BUF];
        ssize_t nread, total_bytes = 0;

        while ((nread = read(fdFR, buf, MAX_BUF)) > 0) {
            write(fdTO, buf, nread);
            total_bytes += nread;
        }

        write(STDOUT_FILENO, "STOR ", 6);
        write(STDOUT_FILENO, filename, strlen(filename));
        write(STDOUT_FILENO, "\nOK: ", 5);
        itoc(total_bytes);
        write(STDOUT_FILENO, " bytes copied to server_root (expected ", 39);
        itoc(total_bytes);
        write(STDOUT_FILENO, " bytes)\n", 8);

        close(fdFR); close(fdTO);
    }
    else if (strncmp(FTPcommand, "QUIT", 4) == 0) {
        write(STDOUT_FILENO, "Quit success\n", 14);
    }

    exit(0);
}

/////////////////////////////////////////////////////////////////////////
// itoc                                                                // 
// =================================================================== //
// Input: long n -> Integer value to convert                           //
//                                                                     //
// Output: void                                                        //
//                                                                     //
// Purpose:                                                            //
// Converts an integer to an ASCII string                              //
// and writes it directly to standard output (STDOUT).                 //      
//                                                                     //
/////////////////////////////////////////////////////////////////////////
void itoc(long n) {
    char buf[20];
    int i = 0;
    if (n == 0) buf[i++] = '0';
    else {
        while (n > 0 && i < 20) {
            buf[i++] = (n % 10) + '0';
            n /= 10;
        }
    }
    // 역순으로 저장된 숫자를 뒤집어서 write
    while (i > 0) {
        write(STDOUT_FILENO, &buf[--i], 1);
    }
}

/////////////////////////////////////////////////////////////////////////
// writePermissions                                                    // 
// =================================================================== //
// Input: mode_t mode -> File status information                       //
//                                                                     //
// Output: void                                                        //
//                                                                     //
// Purpose:                                                            //
// Interprets the file mode bits                                       //
// and outputs a 10-digit permission string (e.g., drwxr-xr-x).        //      
//                                                                     //
/////////////////////////////////////////////////////////////////////////
void writePermissions(mode_t mode) {
    char perm[10];
    perm[0] = (S_ISDIR(mode)) ? 'd' : '-';
    perm[1] = (mode & S_IRUSR) ? 'r' : '-';
    perm[2] = (mode & S_IWUSR) ? 'w' : '-';
    perm[3] = (mode & S_IXUSR) ? 'x' : '-';
    perm[4] = (mode & S_IRGRP) ? 'r' : '-';
    perm[5] = (mode & S_IWGRP) ? 'w' : '-';
    perm[6] = (mode & S_IXGRP) ? 'x' : '-';
    perm[7] = (mode & S_IROTH) ? 'r' : '-';
    perm[8] = (mode & S_IWOTH) ? 'w' : '-';
    perm[9] = (mode & S_IXOTH) ? 'x' : '-';
    write(STDOUT_FILENO, perm, 10);
    write(STDOUT_FILENO, " ", 1);
}

/////////////////////////////////////////////////////////////////////////
// myls                                                                // 
// =================================================================== //
// Input: char *option -> String of ls execution options and arguments //
//        char *path -> Path to the target directory or file to list   //
//                                                                     //
// Output: void                                                        //
//                                                                     //
// Purpose:                                                            //
// Outputs a list of files in the specified path,                      //
// sorted and formatted according to the options.                      //      
//                                                                     //
/////////////////////////////////////////////////////////////////////////
void myls(char *option, char *path) {
    struct dirent **namelist;
    struct stat st;
    int n;

    if (stat(path, &st) == -1) {
        if (errno == ENOENT) write(STDOUT_FILENO, "Error: no such file or directory\n", 34);
        else write(STDOUT_FILENO, "Error: cannot access\n", 21);
        exit(1);
    }

    int show_all = (option && strstr(option, "a"));
    int long_fmt = (option && strstr(option, "l"));
    int print_count = 0;

    // 디렉토리가 아닌 단일 파일인 경우의 처리
    if (!S_ISDIR(st.st_mode)) {
        if (long_fmt) {
            writePermissions(st.st_mode);
            
            itoc(st.st_nlink); write(STDOUT_FILENO, " ", 1);
            
            char *user = getpwuid(st.st_uid)->pw_name;
            write(STDOUT_FILENO, user, strlen(user)); write(STDOUT_FILENO, " ", 1);
            
            char *group = getgrgid(st.st_gid)->gr_name;
            write(STDOUT_FILENO, group, strlen(group)); write(STDOUT_FILENO, " ", 1);
            
            itoc(st.st_size); write(STDOUT_FILENO, " ", 1);
            
            char *time_str = ctime(&st.st_mtime);
            write(STDOUT_FILENO, time_str + 4, 12);
            write(STDOUT_FILENO, " ", 1);

            write(STDOUT_FILENO, path, strlen(path));
            write(STDOUT_FILENO, "\n", 1);
        } else {
            write(STDOUT_FILENO, path, strlen(path));
            write(STDOUT_FILENO, "\n", 1);
        }
        exit(0); // 단일 파일 처리 후 종료
    }

    // scandir로 ASCII 오름차순 리스트 획득 (n은 항목 개수)
    n = scandir(path, &namelist, NULL, alphasort);
    if (n < 0) {
        write(STDOUT_FILENO, "Error: cannot access\n", 21);
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        if (!show_all && namelist[i]->d_name[0] == '.') {
            free(namelist[i]);
            continue;
        }

        struct stat fst;
        char fullpath[MAX_BUF];
        int p_len = strlen(path);
        memcpy(fullpath, path, p_len);
        fullpath[p_len] = '/';
        strcpy(fullpath + p_len + 1, namelist[i]->d_name);
        
        lstat(fullpath, &fst);

        if (long_fmt) {
            writePermissions(fst.st_mode);
            
            itoc(fst.st_nlink); write(STDOUT_FILENO, " ", 1);
            
            char *user = getpwuid(fst.st_uid)->pw_name;
            write(STDOUT_FILENO, user, strlen(user)); write(STDOUT_FILENO, " ", 1);
            
            char *group = getgrgid(fst.st_gid)->gr_name;
            write(STDOUT_FILENO, group, strlen(group)); write(STDOUT_FILENO, " ", 1);
            
            itoc(fst.st_size); write(STDOUT_FILENO, " ", 1);
            
            // 시간 처리
            char *time_str = ctime(&fst.st_mtime);
            write(STDOUT_FILENO, time_str + 4, 12); 
            write(STDOUT_FILENO, " ", 1);

            write(STDOUT_FILENO, namelist[i]->d_name, strlen(namelist[i]->d_name));
            if (S_ISDIR(fst.st_mode)) write(STDOUT_FILENO, "/", 1);
            write(STDOUT_FILENO, "\n", 1);
        } else {
            // 한 줄에 5개까지 출력
            write(STDOUT_FILENO, namelist[i]->d_name, strlen(namelist[i]->d_name));
            if (S_ISDIR(fst.st_mode)) write(STDOUT_FILENO, "/", 1);

            print_count++;
            if (print_count % 5 == 0) write(STDOUT_FILENO, "\n", 1);
            else write(STDOUT_FILENO, "\t", 1);
        }
        free(namelist[i]);
    }

    if (!long_fmt && print_count % 5 != 0) write(STDOUT_FILENO, "\n", 1);
    free(namelist);
    
    exit(0);
}