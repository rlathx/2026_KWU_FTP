#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define MAX_BUFF 1024
#define SEND_BUFF 8192
#define MAX_BUF 1024

int client_info(struct sockaddr_in *cliaddr);
int cmd_process(char *buff, char *result_buff);

void itoc(long n, char *result_buff);
void writePermissions(mode_t mode, char *result_buff);
int myls(char *option, char *path, char *result_buff);

///////////////////////////////////////////////////////////////////////////
// File Name    :srv.c                                                   //
// Date         :2026/05/01                                              //
// OS       :Ubuntu 20.04.6 LTS 64bits                                   //
// Author   : Kim Tae Hyeon                                              //
// Student ID   :2024402034                                              //
//  -------------------------------------------------------------------- //
// Title    :System Programming Assignment #2-1 ( ftp server )           //
// Description:                                                          //
// This program implements a simple FTP server using TCP socket.         //
// The server receives converted FTP commands from the client,           //
// processes NLST and QUIT commands, and sends the result back           //
// to the client.                                                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// main                                                                //
// =================================================================== //
// Input: int argc -> Number of command line arguments                 //
//        char **argv -> Command line arguments                        //
//                                                                     //
// Output: int                                                         //
//                                                                     //
// Purpose:                                                            //
// Creates a TCP server socket, binds it to the given port,            //
// waits for client connection, receives FTP commands from client,     //
// sends processed results back to client, and terminates server       //
// when QUIT command is received.                                      //
//                                                                     //
/////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    char buff[MAX_BUFF], result_buff[SEND_BUFF];
    int n;

    if (argc != 2)
    {
        write(STDOUT_FILENO, "입력 규격: ./srv [PORT]\n", 29);
        exit(1);
    }

    /* open socket and listen */
    int serverfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;

    serverfd = socket(PF_INET, SOCK_STREAM, 0);
    if (serverfd < 0)
    {
        write(STDERR_FILENO, "socket() error!!\n", 18);
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = PF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(serverfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        write(STDERR_FILENO, "bind() error!!\n", 16);
        close(serverfd);
        exit(1);
    }

    if (listen(serverfd, 5) < 0)
    {
        write(STDERR_FILENO, "listen() error!!\n", 18);
        close(serverfd);
        exit(1);
    }

    for (;;)
    {
        clilen = sizeof(cliaddr);
        connfd = accept(serverfd, (struct sockaddr *)&cliaddr, &clilen);

        if (connfd < 0)
        {
            write(STDERR_FILENO, "accept() error!!\n", strlen("accept() error!!\n"));
            continue;
        }

        if (client_info(&cliaddr) < 0) /* display client ip and port */
            write(STDERR_FILENO, "client_info() err!!\n", strlen("client_info() err!!\n"));

        while (1)
        {
            memset(buff, 0, MAX_BUFF);
            memset(result_buff, 0, SEND_BUFF);

            n = read(connfd, buff, MAX_BUFF - 1);
            buff[n] = '\0';

            if (cmd_process(buff, result_buff) < 0)
            {
                write(STDERR_FILENO, "cmd_process() err!!\n", 21);
                break;
            }

            write(connfd, result_buff, strlen(result_buff));

            if (!strcmp(result_buff, "QUIT"))
            {
                write(STDOUT_FILENO, "QUIT\n", 6);
                close(connfd);
                close(serverfd);
                return 0;
            }
        }
    }

    close(serverfd);
    return 0;
}

/////////////////////////////////////////////////////////////////////////
// client_info                                                         //
// =================================================================== //
// Input: struct sockaddr_in *cliaddr -> Client socket address          //
//                                                                     //
// Output: int                                                         //
//                                                                     //
// Purpose:                                                            //
// Displays connected client's IP address and port number.             //
//                                                                     //
/////////////////////////////////////////////////////////////////////////
int client_info(struct sockaddr_in *cliaddr)
{
    char port_buff[100];

    write(STDOUT_FILENO, "==========Client info==========\n",
          strlen("==========Client info==========\n"));

    write(STDOUT_FILENO, "client IP: ", strlen("client IP: "));
    write(STDOUT_FILENO, inet_ntoa(cliaddr->sin_addr),
          strlen(inet_ntoa(cliaddr->sin_addr)));
    write(STDOUT_FILENO, "\n", 1);

    sprintf(port_buff, "client port: %d\n", ntohs(cliaddr->sin_port));
    write(STDOUT_FILENO, port_buff, strlen(port_buff));

    write(STDOUT_FILENO, "===============================\n",
          strlen("===============================\n"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////
// cmd_process                                                         //
// =================================================================== //
// Input: char *buff -> FTP command received from client                //
//        char *result_buff -> Buffer to store command result           //
//                                                                     //
// Output: int                                                         //
//                                                                     //
// Purpose:                                                            //
// Processes converted FTP command and stores the result.               //
//                                                                     //
/////////////////////////////////////////////////////////////////////////
int cmd_process(char *buff, char *result_buff)
{
    char FTPcommand[MAX_BUFF];

    memset(FTPcommand, 0, MAX_BUFF);
    strcpy(FTPcommand, buff);

    if (strncmp(FTPcommand, "QUIT", 4) == 0)
    {
        strcpy(result_buff, "QUIT");
        return 0;
    }

    if (strncmp(FTPcommand, "NLST", 4) == 0)
    {
        char *option = NULL;
        char *path = ".";

        /* 명령어 "NLST" 이후의 인자 파싱 */
        char *token = strtok(FTPcommand, " \n"); // "NLST"

        while ((token = strtok(NULL, " \n")) != NULL)
        {
            if (token[0] == '-')
                option = token;
            else
                path = token;
        }

        /* 서버 화면에 변환된 명령어 출력 */
        write(STDOUT_FILENO, "NLST", 4);

        if (option != NULL)
        {
            write(STDOUT_FILENO, " ", 1);
            write(STDOUT_FILENO, option, strlen(option));
        }

        if (path != NULL && strcmp(path, ".") != 0)
        {
            write(STDOUT_FILENO, " ", 1);
            write(STDOUT_FILENO, path, strlen(path));
        }

        write(STDOUT_FILENO, "\n", 1);

        myls(option, path, result_buff);
        return 0;
    }

    strcpy(result_buff, "Error: invalid command\n");
    return 0;
}

/////////////////////////////////////////////////////////////////////////
// itoc                                                                //
// =================================================================== //
// Input: long n -> Integer value to convert                           //
//        char *result_buff -> Buffer to store converted result         //
//                                                                     //
// Output: void                                                        //
//                                                                     //
// Purpose:                                                            //
// Converts an integer to an ASCII string                              //
// and appends it to result buffer.                                    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////
void itoc(long n, char *result_buff)
{
    char buf[20];
    int i = 0;

    if (n == 0)
        buf[i++] = '0';
    else
    {
        while (n > 0 && i < 20)
        {
            buf[i++] = (n % 10) + '0';
            n /= 10;
        }
    }

    /* 역순으로 저장된 숫자를 뒤집어서 result_buff에 저장 */
    while (i > 0)
    {
        int len = strlen(result_buff);
        result_buff[len] = buf[--i];
        result_buff[len + 1] = '\0';
    }
}

/////////////////////////////////////////////////////////////////////////
// writePermissions                                                    //
// =================================================================== //
// Input: mode_t mode -> File status information                       //
//        char *result_buff -> Buffer to store permission string        //
//                                                                     //
// Output: void                                                        //
//                                                                     //
// Purpose:                                                            //
// Interprets the file mode bits                                       //
// and appends a 10-digit permission string to result buffer.          //
//                                                                     //
/////////////////////////////////////////////////////////////////////////
void writePermissions(mode_t mode, char *result_buff)
{
    char perm[11];

    if (S_ISDIR(mode))
        perm[0] = 'd';
    else if (S_ISLNK(mode))
        perm[0] = 'l';
    else
        perm[0] = '-';

    perm[1] = (mode & S_IRUSR) ? 'r' : '-';
    perm[2] = (mode & S_IWUSR) ? 'w' : '-';
    perm[3] = (mode & S_IXUSR) ? 'x' : '-';
    perm[4] = (mode & S_IRGRP) ? 'r' : '-';
    perm[5] = (mode & S_IWGRP) ? 'w' : '-';
    perm[6] = (mode & S_IXGRP) ? 'x' : '-';
    perm[7] = (mode & S_IROTH) ? 'r' : '-';
    perm[8] = (mode & S_IWOTH) ? 'w' : '-';
    perm[9] = (mode & S_IXOTH) ? 'x' : '-';

    perm[10] = '\0';

    strcat(result_buff, perm);
    strcat(result_buff, " ");
}

/////////////////////////////////////////////////////////////////////////
// myls                                                                //
// =================================================================== //
// Input: char *option -> ls option                                    //
//        char *path -> target path                                    //
//        char *result_buff -> Buffer to store ls result                //
//                                                                     //
// Output: int                                                         //
//                                                                     //
// Purpose:                                                            //
// Processes NLST command like ls, ls -a, ls -l, ls -al.                //
//                                                                     //
/////////////////////////////////////////////////////////////////////////
int myls(char *option, char *path, char *result_buff)
{
    struct dirent **namelist;
    struct stat st;
    int n;

    result_buff[0] = '\0';

    if (stat(path, &st) == -1)
    {
        if (errno == ENOENT)
            strcpy(result_buff, "Error: no such file or directory\n");
        else
            strcpy(result_buff, "Error: cannot access\n");

        return -1;
    }

    int show_all = (option && strstr(option, "a"));
    int long_fmt = (option && strstr(option, "l"));
    int print_count = 0;

    /* 디렉토리가 아닌 단일 파일인 경우의 처리 */
    if (!S_ISDIR(st.st_mode))
    {
        if (long_fmt)
        {
            writePermissions(st.st_mode, result_buff);

            itoc(st.st_nlink, result_buff);
            strcat(result_buff, " ");

            struct passwd *pw = getpwuid(st.st_uid);
            if (pw != NULL)
                strcat(result_buff, pw->pw_name);
            else
                itoc(st.st_uid, result_buff);
            strcat(result_buff, " ");

            struct group *gr = getgrgid(st.st_gid);
            if (gr != NULL)
                strcat(result_buff, gr->gr_name);
            else
                itoc(st.st_gid, result_buff);
            strcat(result_buff, " ");

            itoc(st.st_size, result_buff);
            strcat(result_buff, " ");

            char *time_str = ctime(&st.st_mtime);
            strncat(result_buff, time_str + 4, 12);
            strcat(result_buff, " ");

            strcat(result_buff, path);
            strcat(result_buff, "\n");
        }
        else
        {
            strcat(result_buff, path);
            strcat(result_buff, "\n");
        }

        return 0;
    }

    /* scandir로 ASCII 오름차순 리스트 획득 */
    n = scandir(path, &namelist, NULL, alphasort);
    if (n < 0)
    {
        strcpy(result_buff, "Error: cannot access\n");
        return -1;
    }

    for (int i = 0; i < n; i++)
    {
        if (!show_all && namelist[i]->d_name[0] == '.')
        {
            free(namelist[i]);
            continue;
        }

        struct stat fst;
        char fullpath[MAX_BUF];

        memset(fullpath, 0, MAX_BUF);

        int p_len = strlen(path);
        memcpy(fullpath, path, p_len);
        fullpath[p_len] = '/';
        strcpy(fullpath + p_len + 1, namelist[i]->d_name);

        lstat(fullpath, &fst);

        if (long_fmt)
        {
            writePermissions(fst.st_mode, result_buff);

            itoc(fst.st_nlink, result_buff);
            strcat(result_buff, " ");

            struct passwd *pw = getpwuid(fst.st_uid);
            if (pw != NULL)
                strcat(result_buff, pw->pw_name);
            else
                itoc(fst.st_uid, result_buff);
            strcat(result_buff, " ");

            struct group *gr = getgrgid(fst.st_gid);
            if (gr != NULL)
                strcat(result_buff, gr->gr_name);
            else
                itoc(fst.st_gid, result_buff);
            strcat(result_buff, " ");

            itoc(fst.st_size, result_buff);
            strcat(result_buff, " ");

            char *time_str = ctime(&fst.st_mtime);
            strncat(result_buff, time_str + 4, 12);
            strcat(result_buff, " ");

            strcat(result_buff, namelist[i]->d_name);

            if (S_ISDIR(fst.st_mode))
                strcat(result_buff, "/");

            strcat(result_buff, "\n");
        }
        else
        {
            /* 한 줄에 5개까지 출력 */
            strcat(result_buff, namelist[i]->d_name);

            if (S_ISDIR(fst.st_mode))
                strcat(result_buff, "/");

            print_count++;

            if (print_count % 5 == 0)
                strcat(result_buff, "\n");
            else
                strcat(result_buff, "\t");
        }

        free(namelist[i]);
    }

    if (!long_fmt && print_count % 5 != 0)
        strcat(result_buff, "\n");

    free(namelist);

    return 0;
}

