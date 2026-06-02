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

int cmd_process(char *buff, char *result_buff);
char *convert_str_to_addr(char *str, unsigned int *port);

void itoc(long n, char *result_buff);
void writePermissions(mode_t mode, char *result_buff);
int myls(char *option, char *path, char *result_buff);

///////////////////////////////////////////////////////////////////////////
// File Name    : srv.c                                                  //
// Date         : 2026/05/30                                             //
// OS           : Ubuntu 20.04.6 LTS 64bits                              //
// Author       : Kim Tae Hyeon                                          //
// Student ID   : 2024402034                                             //
// ----------------------------------------------------------------------//
// Title        : System Programming Assignment #3-2 ( ftp server )       //
// Description  :                                                        //
// This program implements a simple FTP server using control connection   //
// and data connection. The server receives PORT and NLST commands        //
// through control connection, connects to client's data port, and sends  //
// ls result through data connection.                                     //
///////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// main                                                                //
// =================================================================== //
// Input: int argc -> Number of command line arguments                  //
//        char **argv -> Command line arguments                         //
//                                                                     //
// Output: int                                                         //
//                                                                     //
// Purpose:                                                            //
// Creates a TCP server socket for control connection.                  //
// After receiving PORT command, server connects to client's data port. //
// After receiving NLST command, server sends ls result through         //
// data connection and sends reply messages through control connection. //
/////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    char buff[MAX_BUFF];
    char result_buff[SEND_BUFF];
    char *host_ip;
    char temp[25];
    unsigned int port_num;
    int n;

    if (argc != 2)
    {
        write(STDOUT_FILENO, "Usage: ./srv [PORT]\n",
              strlen("Usage: ./srv [PORT]\n"));
        exit(1);
    }

    ////////////////////// Make control connection //////////////////////
    int serverfd;
    int connfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;
    socklen_t clilen;

    serverfd = socket(PF_INET, SOCK_STREAM, 0);
    if (serverfd < 0)
    {
        write(STDERR_FILENO, "socket() error!!\n",
              strlen("socket() error!!\n"));
        exit(1);
    }

    int opt = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = PF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(serverfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        write(STDERR_FILENO, "bind() error!!\n",
              strlen("bind() error!!\n"));
        close(serverfd);
        exit(1);
    }

    if (listen(serverfd, 5) < 0)
    {
        write(STDERR_FILENO, "listen() error!!\n",
              strlen("listen() error!!\n"));
        close(serverfd);
        exit(1);
    }

    for (;;)
    {
        clilen = sizeof(cliaddr);
        connfd = accept(serverfd, (struct sockaddr *)&cliaddr, &clilen);

        if (connfd < 0)
        {
            write(STDERR_FILENO, "accept() error!!\n",
                  strlen("accept() error!!\n"));
            continue;
        }

        int datafd = -1;

        while (1)
        {
            memset(buff, 0, MAX_BUFF);
            memset(result_buff, 0, SEND_BUFF);

            n = read(connfd, buff, MAX_BUFF - 1);
            if (n <= 0)
                break;

            buff[n] = '\0';

            ////////////////////////////// PORT //////////////////////////////
            if (strncmp(buff, "PORT", 4) == 0)
            {
                char *p;

                memset(temp, 0, sizeof(temp));

                p = buff + 5;
                strncpy(temp, p, sizeof(temp) - 1);

                if (strlen(temp) > 0 && temp[strlen(temp) - 1] == '\n')
                    temp[strlen(temp) - 1] = '\0';

                write(STDOUT_FILENO, "PORT ", strlen("PORT "));
                write(STDOUT_FILENO, temp, strlen(temp));
                write(STDOUT_FILENO, "\n", 1);

                host_ip = convert_str_to_addr(temp,
                                              (unsigned int *)&port_num);
                if (host_ip == NULL)
                {
                    write(connfd, "500 PORT command failed\n",
                          strlen("500 PORT command failed\n"));
                    continue;
                }

                ////////////////////// Make data connection //////////////////////
                datafd = socket(PF_INET, SOCK_STREAM, 0);
                if (datafd < 0)
                {
                    write(STDERR_FILENO, "data socket() error!!\n",
                          strlen("data socket() error!!\n"));
                    free(host_ip);
                    write(connfd, "500 PORT command failed\n",
                          strlen("500 PORT command failed\n"));
                    continue;
                }

                struct sockaddr_in dataaddr;
                memset(&dataaddr, 0, sizeof(dataaddr));
                dataaddr.sin_family = AF_INET;
                dataaddr.sin_addr.s_addr = inet_addr(host_ip);
                dataaddr.sin_port = htons(port_num);

                if (connect(datafd, (struct sockaddr *)&dataaddr,
                            sizeof(dataaddr)) < 0)
                {
                    write(STDERR_FILENO, "data connect() error!!\n",
                          strlen("data connect() error!!\n"));
                    close(datafd);
                    datafd = -1;
                    free(host_ip);
                    write(connfd, "500 PORT command failed\n",
                          strlen("500 PORT command failed\n"));
                    continue;
                }

                free(host_ip);

                write(connfd, "200 Port command successful\n",
                      strlen("200 Port command successful\n"));

                write(STDOUT_FILENO, "200 Port command successful\n",
                      strlen("200 Port command successful\n"));
            }

            ////////////////////////////// NLST //////////////////////////////
            else if (strncmp(buff, "NLST", 4) == 0)
            {
                if (datafd < 0)
                {
                    write(connfd, "425 No data connection\n",
                          strlen("425 No data connection\n"));
                    continue;
                }

                /*
                    제안서 example result와 출력 순서를 맞추기 위해
                    cmd_process()를 먼저 호출한다.
                    cmd_process() 내부에서 서버 콘솔에 NLST가 출력된다.
                */
                if (cmd_process(buff, result_buff) < 0)
                {
                    write(STDERR_FILENO, "cmd_process() err!!\n",
                          strlen("cmd_process() err!!\n"));
                }

                write(connfd,
                      "150 Opening data connection for directory list.\n",
                      strlen("150 Opening data connection for directory list.\n"));

                write(STDOUT_FILENO,
                      "150 Opening data connection for directory list\n",
                      strlen("150 Opening data connection for directory list\n"));

                write(datafd, result_buff, strlen(result_buff));

                close(datafd);
                datafd = -1;

                write(connfd, "226 Result is sent successfully.\n",
                      strlen("226 Result is sent successfully.\n"));

                write(STDOUT_FILENO, "226 Result is sent successfully.\n",
                      strlen("226 Result is sent successfully.\n"));

                close(connfd);
                close(serverfd);
                return 0;
            }

            ////////////////////////////// QUIT //////////////////////////////
            else if (strncmp(buff, "QUIT", 4) == 0)
            {
                if (datafd >= 0)
                    close(datafd);

                write(STDOUT_FILENO, "QUIT\n", strlen("QUIT\n"));
                close(connfd);
                close(serverfd);
                return 0;
            }

            ////////////////////////// Invalid command ////////////////////////
            else
            {
                write(connfd, "500 Invalid command\n",
                      strlen("500 Invalid command\n"));
            }
        }

        if (datafd >= 0)
            close(datafd);

        close(connfd);
    }

    close(serverfd);
    return 0;
}

/////////////////////////////////////////////////////////////////////////
// convert_str_to_addr                                                 //
// =================================================================== //
// Input: char *str -> PORT command argument string                    //
//        unsigned int *port -> Port number storage                    //
//                                                                     //
// Output: char* -> Converted IP address string                        //
//                                                                     //
// Purpose:                                                            //
// Converts h1,h2,h3,h4,p1,p2 format into IP address string            //
// and port number.                                                    //
/////////////////////////////////////////////////////////////////////////
char *convert_str_to_addr(char *str, unsigned int *port)
{
    char *addr;
    unsigned int h1, h2, h3, h4, p1, p2;

    addr = (char *)malloc(25);
    if (addr == NULL)
        return NULL;

    if (sscanf(str, "%u,%u,%u,%u,%u,%u",
               &h1, &h2, &h3, &h4, &p1, &p2) != 6)
    {
        free(addr);
        return NULL;
    }

    sprintf(addr, "%u.%u.%u.%u", h1, h2, h3, h4);
    *port = p1 * 256 + p2;

    return addr;
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
// Processes NLST command and stores ls result into result buffer.      //
/////////////////////////////////////////////////////////////////////////
int cmd_process(char *buff, char *result_buff)
{
    char FTPcommand[MAX_BUFF];

    memset(FTPcommand, 0, MAX_BUFF);
    strcpy(FTPcommand, buff);

    if (strncmp(FTPcommand, "NLST", 4) == 0)
    {
        char *option = NULL;
        char *path = ".";

        char *token = strtok(FTPcommand, " \n");

        while ((token = strtok(NULL, " \n")) != NULL)
        {
            if (token[0] == '-')
                option = token;
            else
                path = token;
        }

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
// Converts an integer to an ASCII string and appends it to buffer.     //
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
// Interprets file mode bits and appends permission string to buffer.   //
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
// Processes NLST command like ls, ls -a, ls -l, and ls -al.            //
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
            /*
                Assignment 3-2 example result prints one file per line.
                Therefore, normal ls result is stored with newline
                after every file name.
            */
            strcat(result_buff, namelist[i]->d_name);

            if (S_ISDIR(fst.st_mode))
                strcat(result_buff, "/");

            strcat(result_buff, "\n");
        }

        free(namelist[i]);
    }

    free(namelist);

    return 0;
}