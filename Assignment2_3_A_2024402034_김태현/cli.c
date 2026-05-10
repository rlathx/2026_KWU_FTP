#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

#define MAX_BUFF 1024
#define RCV_BUFF 8192

int conv_cmd(char *buff, char *cmd_buff);
void process_result(char *rcv_buff);

int sigintTargetSockfd = -1;

void sigintHandler(int signum)
{
    char quit_msg[] = "QUIT";

    if (sigintTargetSockfd >= 0)
    {
        write(sigintTargetSockfd, quit_msg, strlen(quit_msg));
        close(sigintTargetSockfd);
    }

    write(STDOUT_FILENO, "\nProgram quit!!\n", strlen("\nProgram quit!!\n"));
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        write(STDOUT_FILENO, "입력 규격: ./cli [IP] [PORT]\n",
              strlen("입력 규격: ./cli [IP] [PORT]\n"));
        exit(1);
    }

    char buff[MAX_BUFF];
    char cmd_buff[MAX_BUFF];
    char rcv_buff[RCV_BUFF];
    int n;

    int sockfd;
    struct sockaddr_in srvaddr;

    signal(SIGINT, sigintHandler);

    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        write(STDERR_FILENO, "socket() error!!\n",
              strlen("socket() error!!\n"));
        exit(1);
    }

    sigintTargetSockfd = sockfd;

    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = PF_INET;
    srvaddr.sin_addr.s_addr = inet_addr(argv[1]);
    srvaddr.sin_port = htons(atoi(argv[2]));

    if (connect(sockfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr)) < 0)
    {
        write(STDOUT_FILENO, "connect() error!!\n",
              strlen("connect() error!!\n"));
        close(sockfd);
        exit(1);
    }

    while (1)
    {
        memset(buff, 0, MAX_BUFF);
        memset(cmd_buff, 0, MAX_BUFF);
        memset(rcv_buff, 0, RCV_BUFF);

        write(STDOUT_FILENO, "> ", 2);

        n = read(STDIN_FILENO, buff, MAX_BUFF - 1);
        if (n <= 0)
            break;

        buff[n] = '\0';

        if (conv_cmd(buff, cmd_buff) < 0)
        {
            write(STDERR_FILENO, "conv_cmd() error!!\n",
                  strlen("conv_cmd() error!!\n"));
            continue;
        }

        n = strlen(cmd_buff);

        if (write(sockfd, cmd_buff, n) != n)
        {
            write(STDERR_FILENO, "write() error!!\n",
                  strlen("write() error!!\n"));
            break;
        }

        n = read(sockfd, rcv_buff, RCV_BUFF - 1);
        if (n <= 0)
        {
            write(STDERR_FILENO, "read() error\n",
                  strlen("read() error\n"));
            break;
        }

        rcv_buff[n] = '\0';

        if (strcmp(rcv_buff, "QUIT") == 0)
        {
            write(STDOUT_FILENO, "Program quit!!\n",
                  strlen("Program quit!!\n"));
            break;
        }

        process_result(rcv_buff);
    }

    close(sockfd);
    return 0;
}

int conv_cmd(char *buff, char *cmd_buff)
{
    char *argv[30];
    int argc = 0;
    char temp[MAX_BUFF];

    memset(temp, 0, MAX_BUFF);
    strcpy(temp, buff);

    char *ptr = strtok(temp, " \n");
    while (ptr != NULL)
    {
        if (argc >= 30)
            return -1;

        argv[argc++] = ptr;
        ptr = strtok(NULL, " \n");
    }

    if (argc == 0)
        return -1;

    if (strcmp(argv[0], "ls") == 0)
    {
        strcpy(cmd_buff, "NLST");

        int path_count = 0;
        char *path = NULL;
        int has_a = 0;
        int has_l = 0;

        for (int i = 1; i < argc; i++)
        {
            if (strchr(argv[i], '*') != NULL)
            {
                write(STDERR_FILENO,
                      "Error: wildcard (*) is not supported\n",
                      strlen("Error: wildcard (*) is not supported\n"));
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
                    {
                        write(STDERR_FILENO,
                              "Error: invalid option\n",
                              strlen("Error: invalid option\n"));
                        return -1;
                    }
                }
            }
            else
            {
                path_count++;

                if (path_count > 1)
                {
                    write(STDERR_FILENO,
                          "Error: only one path allowed\n",
                          strlen("Error: only one path allowed\n"));
                    return -1;
                }

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

        if (path != NULL)
        {
            strcat(cmd_buff, " ");
            strcat(cmd_buff, path);
        }

        return 0;
    }

    else if (strcmp(argv[0], "dir") == 0)
    {
        strcpy(cmd_buff, "LIST");

        if (argc > 2)
        {
            write(STDERR_FILENO,
                  "Error: only one path allowed\n",
                  strlen("Error: only one path allowed\n"));
            return -1;
        }

        if (argc == 2)
        {
            if (argv[1][0] == '-')
            {
                write(STDERR_FILENO,
                      "Error: invalid option\n",
                      strlen("Error: invalid option\n"));
                return -1;
            }

            strcat(cmd_buff, " ");
            strcat(cmd_buff, argv[1]);
        }

        return 0;
    }

    else if (strcmp(argv[0], "pwd") == 0)
    {
        if (argc != 1)
        {
            if (argv[1][0] == '-')
                write(STDERR_FILENO,
                      "Error: invalid option\n",
                      strlen("Error: invalid option\n"));
            else
                write(STDERR_FILENO,
                      "Error: argument is not required\n",
                      strlen("Error: argument is not required\n"));

            return -1;
        }

        strcpy(cmd_buff, "PWD");
        return 0;
    }

    else if (strcmp(argv[0], "cd") == 0)
    {
        if (argc < 2)
        {
            write(STDERR_FILENO,
                  "Error: argument is required\n",
                  strlen("Error: argument is required\n"));
            return -1;
        }

        if (argc > 2)
        {
            write(STDERR_FILENO,
                  "Error: only one argument is allowed\n",
                  strlen("Error: only one argument is allowed\n"));
            return -1;
        }

        if (argv[1][0] == '-')
        {
            write(STDERR_FILENO,
                  "Error: invalid option\n",
                  strlen("Error: invalid option\n"));
            return -1;
        }

        if (strcmp(argv[1], "..") == 0)
        {
            strcpy(cmd_buff, "CDUP");
        }
        else
        {
            strcpy(cmd_buff, "CWD ");
            strcat(cmd_buff, argv[1]);
        }

        return 0;
    }

    else if (strcmp(argv[0], "mkdir") == 0)
    {
        if (argc < 2)
        {
            write(STDERR_FILENO,
                  "Error: argument is required\n",
                  strlen("Error: argument is required\n"));
            return -1;
        }

        strcpy(cmd_buff, "MKD");

        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                write(STDERR_FILENO,
                      "Error: invalid option\n",
                      strlen("Error: invalid option\n"));
                return -1;
            }

            strcat(cmd_buff, " ");
            strcat(cmd_buff, argv[i]);
        }

        return 0;
    }

    else if (strcmp(argv[0], "delete") == 0)
    {
        if (argc < 2)
        {
            write(STDERR_FILENO,
                  "Error: argument is required\n",
                  strlen("Error: argument is required\n"));
            return -1;
        }

        strcpy(cmd_buff, "DELE");

        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                write(STDERR_FILENO,
                      "Error: invalid option\n",
                      strlen("Error: invalid option\n"));
                return -1;
            }

            strcat(cmd_buff, " ");
            strcat(cmd_buff, argv[i]);
        }

        return 0;
    }

    else if (strcmp(argv[0], "rmdir") == 0)
    {
        if (argc < 2)
        {
            write(STDERR_FILENO,
                  "Error: argument is required\n",
                  strlen("Error: argument is required\n"));
            return -1;
        }

        strcpy(cmd_buff, "RMD");

        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                write(STDERR_FILENO,
                      "Error: invalid option\n",
                      strlen("Error: invalid option\n"));
                return -1;
            }

            strcat(cmd_buff, " ");
            strcat(cmd_buff, argv[i]);
        }

        return 0;
    }

    else if (strcmp(argv[0], "rename") == 0)
    {
        if (argc != 3)
        {
            write(STDERR_FILENO,
                  "Error: two arguments are required\n",
                  strlen("Error: two arguments are required\n"));
            return -1;
        }

        if (argv[1][0] == '-' || argv[2][0] == '-')
        {
            write(STDERR_FILENO,
                  "Error: invalid option\n",
                  strlen("Error: invalid option\n"));
            return -1;
        }

        strcpy(cmd_buff, "RNFR ");
        strcat(cmd_buff, argv[1]);
        strcat(cmd_buff, " RNTO ");
        strcat(cmd_buff, argv[2]);

        return 0;
    }

    else if (strcmp(argv[0], "get") == 0)
    {
        if (argc != 2)
        {
            write(STDERR_FILENO,
                  "Error: argument is required\n",
                  strlen("Error: argument is required\n"));
            return -1;
        }

        if (argv[1][0] == '-')
        {
            write(STDERR_FILENO,
                  "Error: invalid option\n",
                  strlen("Error: invalid option\n"));
            return -1;
        }

        strcpy(cmd_buff, "RETR ");
        strcat(cmd_buff, argv[1]);

        return 0;
    }

    else if (strcmp(argv[0], "put") == 0)
    {
        if (argc != 2)
        {
            write(STDERR_FILENO,
                  "Error: argument is required\n",
                  strlen("Error: argument is required\n"));
            return -1;
        }

        if (argv[1][0] == '-')
        {
            write(STDERR_FILENO,
                  "Error: invalid option\n",
                  strlen("Error: invalid option\n"));
            return -1;
        }

        char client_path[MAX_BUFF] = "client_root/";
        strcat(client_path, argv[1]);

        if (access(client_path, F_OK) == -1)
        {
            write(STDERR_FILENO, "Error: '", 8);
            write(STDERR_FILENO, argv[1], strlen(argv[1]));
            write(STDERR_FILENO,
                  "' does not exist in client_root\n",
                  strlen("' does not exist in client_root\n"));
            return -1;
        }

        strcpy(cmd_buff, "STOR ");
        strcat(cmd_buff, argv[1]);

        return 0;
    }

    else if (strcmp(argv[0], "quit") == 0)
    {
        if (argc != 1)
        {
            if (argv[1][0] == '-')
                write(STDERR_FILENO,
                      "Error: invalid option\n",
                      strlen("Error: invalid option\n"));
            else
                write(STDERR_FILENO,
                      "Error: argument is not required\n",
                      strlen("Error: argument is not required\n"));

            return -1;
        }

        strcpy(cmd_buff, "QUIT");
        return 0;
    }

    return -1;
}

void process_result(char *rcv_buff)
{
    write(STDOUT_FILENO, rcv_buff, strlen(rcv_buff));
}