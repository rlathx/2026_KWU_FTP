#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_BUFF 1024
#define RCV_BUFF 4096

int conv_cmd(char *buff, char *cmd_buff);
void process_result(char *rcv_buff);

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        write(STDOUT_FILENO, "입력 규격: ./cli [IP] [PORT]\n", 34);
        exit(1);
    }

    char buff[MAX_BUFF], cmd_buff[MAX_BUFF], rcv_buff[RCV_BUFF];
    int n;

    /* open socket and connect to server */
    int sockfd;
    struct sockaddr_in srvaddr;

    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        write(STDERR_FILENO, "socket() error!!\n", 18);
        exit(1);
    }

    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = PF_INET;
    srvaddr.sin_addr.s_addr = inet_addr(argv[1]);
    srvaddr.sin_port = htons(atoi(argv[2]));

    if (connect(sockfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr)) < 0)
    {
        write(STDOUT_FILENO, "connect() error!!\n", 19);
        close(sockfd);
        exit(1);
    }

    for (;;)
    {
        memset(buff, 0, MAX_BUFF);
        memset(cmd_buff, 0, MAX_BUFF);
        memset(rcv_buff, 0, RCV_BUFF);

        write(STDOUT_FILENO, "> ", 2);

        n = read(STDIN_FILENO, buff, MAX_BUFF - 1);
        if (n <= 0)
            break;

        buff[n] = '\0';

        /* convert ls (including options) to NLST (including options) */
        if (conv_cmd(buff, cmd_buff) < 0)
        {
            write(STDERR_FILENO, "conv_cmd() error!!\n", 20);
            continue;
        }

        n = strlen(cmd_buff);

        if (write(sockfd, cmd_buff, n) != n)
        {
            write(STDERR_FILENO, "write() error!!\n", 17);
            exit(1);
        }

        if ((n = read(sockfd, rcv_buff, RCV_BUFF - 1)) < 0)
        {
            write(STDERR_FILENO, "read() error\n", strlen("read() error\n"));
            exit(1);
        }

        rcv_buff[n] = '\0';

        if (!strcmp(rcv_buff, "QUIT"))
        {
            write(STDOUT_FILENO, "Program quit!!\n", strlen("Program quit!!\n"));
            break;
        }

        /* display ls (including options) command result */
        process_result(rcv_buff);
    }

    close(sockfd);
    return 0;
}

int conv_cmd(char *buff, char *cmd_buff)
{
    char *argv[10];
    int argc = 0;
    char temp[MAX_BUFF];

    memset(temp, 0, MAX_BUFF);
    strcpy(temp, buff);

    /* 공백 기준 파싱 */
    char *ptr = strtok(temp, " \n");
    while (ptr != NULL)
    {
        if (argc >= 10)
            return -1;

        argv[argc++] = ptr;
        ptr = strtok(NULL, " \n");
    }

    if (argc == 0)
        return -1;

    /* ls 명령어 처리 */
    if (strcmp(argv[0], "ls") == 0)
    {
        strcpy(cmd_buff, "NLST");

        int has_a = 0, has_l = 0;
        char *path = NULL;

        for (int i = 1; i < argc; i++)
        {
            if (strchr(argv[i], '*') != NULL)
            {
                write(STDERR_FILENO, "Error: wildcard (*) not supported\n",
                      strlen("Error: wildcard (*) not supported\n"));
                return -1;
            }

            if (argv[i][0] == '-')
            {
                for (int j = 1; j < strlen(argv[i]); j++)
                {
                    if (argv[i][j] == 'a')
                        has_a = 1;
                    else if (argv[i][j] == 'l')
                        has_l = 1;
                    else
                        return -1;
                }
            }
            else
            {
                path = argv[i];
            }
        }

        if (has_a || has_l)
        {
            strcat(cmd_buff, " -");

            if (has_a)
                strcat(cmd_buff, "a");

            if (has_l)
                strcat(cmd_buff, "l");
        }

        if (path)
        {
            strcat(cmd_buff, " ");
            strcat(cmd_buff, path);
        }

        return 0;
    }
    /* quit 명령어 처리 */
    else if (strcmp(argv[0], "quit") == 0)
    {
        strcpy(cmd_buff, "QUIT");

        return 0;
    }
}

void process_result(char *rcv_buff)
{
    write(STDOUT_FILENO, rcv_buff, strlen(rcv_buff));
}