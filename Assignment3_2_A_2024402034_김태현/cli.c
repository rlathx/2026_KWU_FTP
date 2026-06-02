#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_BUFF 1024
#define DATA_PORT_MIN 10001
#define DATA_PORT_MAX 30000

char *convert_addr_to_str(unsigned long ip_addr, unsigned int port);

///////////////////////////////////////////////////////////////////////////
// File Name    : cli.c                                                  //
// Date         : 2026/05/30                                             //
// OS           : Ubuntu 20.04.6 LTS 64bits                              //
// Author       : Kim Tae Hyeon                                          //
// Student ID   : 2024402034                                             //
// ----------------------------------------------------------------------//
// Title        : System Programming Assignment #3-2 ( ftp server )       //
// Description  :                                                        //
// This program connects to the FTP server through control connection.    //
// When user enters ls, the client opens a random data port, sends PORT   //
// command to the server, receives the directory list through data        //
// connection, and prints the received result.                            //
///////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// convert_addr_to_str                                                  //
// =================================================================== //
// Input : unsigned long ip_addr -> Client IP address                   //
//         unsigned int port -> Client data port number                 //
//                                                                     //
// Output: char* -> Converted PORT command argument string              //
//                                                                     //
// Purpose:                                                            //
// Converts client's IP address and data port number into               //
// h1,h2,h3,h4,p1,p2 format used by PORT command.                       //
/////////////////////////////////////////////////////////////////////////
char *convert_addr_to_str(unsigned long ip_addr, unsigned int port)
{
    char *addr;
    unsigned char *ip;
    unsigned int port_num;
    unsigned int p1, p2;

    addr = (char *)malloc(50);
    if (addr == NULL)
        return NULL;

    ip = (unsigned char *)&ip_addr;
    port_num = ntohs(port);

    p1 = port_num / 256;
    p2 = port_num % 256;

    sprintf(addr, "%u,%u,%u,%u,%u,%u",
            ip[0], ip[1], ip[2], ip[3], p1, p2);

    return addr;
}

/////////////////////////////////////////////////////////////////////////
// main                                                                //
// =================================================================== //
// Input : int argc -> Number of command line arguments                 //
//         char **argv -> Command line arguments                        //
//                                                                     //
// Output: int                                                         //
//                                                                     //
// Purpose:                                                            //
// Makes control connection with server.                               //
// For ls command, makes data listening socket, sends PORT command,     //
// sends NLST command, receives ls result through data connection,      //
// and receives server reply through control connection.                //
/////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    int control_sock;
    int data_listen_sock;
    int data_conn_sock;
    struct sockaddr_in servaddr;
    struct sockaddr_in dataaddr;
    struct sockaddr_in temp;
    socklen_t temp_len;
    char cmd_buff[MAX_BUFF];
    char recv_buff[MAX_BUFF];
    char send_buff[MAX_BUFF];
    char *hostport;
    int n;
    int data_port;
    int total_bytes;

    if (argc != 3)
    {
        write(STDOUT_FILENO, "Usage: ./cli [IP] [PORT]\n",
              strlen("Usage: ./cli [IP] [PORT]\n"));
        exit(1);
    }

    ////////////////////// Make control connection //////////////////////
    control_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (control_sock < 0)
    {
        write(STDERR_FILENO, "socket() error!!\n",
              strlen("socket() error!!\n"));
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    if (connect(control_sock, (struct sockaddr *)&servaddr,
                sizeof(servaddr)) < 0)
    {
        write(STDERR_FILENO, "connect() error!!\n",
              strlen("connect() error!!\n"));
        close(control_sock);
        exit(1);
    }

    srand((unsigned int)time(NULL));

    while (1)
    {
        memset(cmd_buff, 0, MAX_BUFF);

        write(STDOUT_FILENO, "> ", 2);

        n = read(STDIN_FILENO, cmd_buff, MAX_BUFF - 1);
        if (n <= 0)
            break;

        cmd_buff[n] = '\0';

        if (cmd_buff[n - 1] == '\n')
            cmd_buff[n - 1] = '\0';

        ////////////////////////////// quit //////////////////////////////
        if (strcmp(cmd_buff, "quit") == 0 || strcmp(cmd_buff, "QUIT") == 0)
        {
            strcpy(send_buff, "QUIT\n");
            write(control_sock, send_buff, strlen(send_buff));
            break;
        }

        ////////////////////////////// ls ////////////////////////////////
        if (strncmp(cmd_buff, "ls", 2) == 0)
        {
            data_listen_sock = socket(PF_INET, SOCK_STREAM, 0);
            if (data_listen_sock < 0)
            {
                write(STDERR_FILENO, "data socket() error!!\n",
                      strlen("data socket() error!!\n"));
                continue;
            }

            int opt = 1;
            setsockopt(data_listen_sock, SOL_SOCKET, SO_REUSEADDR,
                       &opt, sizeof(opt));

            ////////////////////// Select random data port //////////////////////
            while (1)
            {
                data_port = DATA_PORT_MIN +
                            rand() % (DATA_PORT_MAX - DATA_PORT_MIN + 1);

                memset(&dataaddr, 0, sizeof(dataaddr));
                dataaddr.sin_family = AF_INET;
                dataaddr.sin_addr.s_addr = htonl(INADDR_ANY);
                dataaddr.sin_port = htons(data_port);

                if (bind(data_listen_sock, (struct sockaddr *)&dataaddr,
                         sizeof(dataaddr)) == 0)
                    break;
            }

            if (listen(data_listen_sock, 1) < 0)
            {
                write(STDERR_FILENO, "data listen() error!!\n",
                      strlen("data listen() error!!\n"));
                close(data_listen_sock);
                continue;
            }

            ////////////////// Get client IP address for PORT command //////////////////
            temp_len = sizeof(temp);
            memset(&temp, 0, sizeof(temp));

            if (getsockname(control_sock, (struct sockaddr *)&temp,
                            &temp_len) < 0)
            {
                write(STDERR_FILENO, "getsockname() error!!\n",
                      strlen("getsockname() error!!\n"));
                close(data_listen_sock);
                continue;
            }

            temp.sin_port = htons(data_port);

            hostport = convert_addr_to_str(temp.sin_addr.s_addr,
                                           temp.sin_port);
            if (hostport == NULL)
            {
                write(STDERR_FILENO, "convert_addr_to_str() error!!\n",
                      strlen("convert_addr_to_str() error!!\n"));
                close(data_listen_sock);
                continue;
            }

            ////////////////// Send PORT command //////////////////
            memset(send_buff, 0, MAX_BUFF);
            sprintf(send_buff, "PORT %s\n", hostport);

            if (write(control_sock, send_buff, strlen(send_buff)) < 0)
            {
                write(STDERR_FILENO, "write() error!!\n",
                      strlen("write() error!!\n"));
                free(hostport);
                close(data_listen_sock);
                continue;
            }

            free(hostport);

            ////////////////// Receive 200 reply //////////////////
            memset(recv_buff, 0, MAX_BUFF);
            n = read(control_sock, recv_buff, MAX_BUFF - 1);
            if (n <= 0)
            {
                write(STDERR_FILENO, "read() error!!\n",
                      strlen("read() error!!\n"));
                close(data_listen_sock);
                break;
            }

            recv_buff[n] = '\0';
            write(STDOUT_FILENO, recv_buff, strlen(recv_buff));

            ////////////////// Send NLST command //////////////////
            memset(send_buff, 0, MAX_BUFF);

            if (strlen(cmd_buff) == 2)
                strcpy(send_buff, "NLST\n");
            else
                sprintf(send_buff, "NLST%s\n", cmd_buff + 2);

            if (write(control_sock, send_buff, strlen(send_buff)) < 0)
            {
                write(STDERR_FILENO, "write() error!!\n",
                      strlen("write() error!!\n"));
                close(data_listen_sock);
                continue;
            }

            ////////////////// Receive 150 reply //////////////////
            memset(recv_buff, 0, MAX_BUFF);
            n = read(control_sock, recv_buff, MAX_BUFF - 1);
            if (n <= 0)
            {
                write(STDERR_FILENO, "read() error!!\n",
                      strlen("read() error!!\n"));
                close(data_listen_sock);
                break;
            }

            recv_buff[n] = '\0';
            write(STDOUT_FILENO, recv_buff, strlen(recv_buff));

            ////////////////// Accept data connection //////////////////
            data_conn_sock = accept(data_listen_sock, NULL, NULL);
            if (data_conn_sock < 0)
            {
                write(STDERR_FILENO, "data accept() error!!\n",
                      strlen("data accept() error!!\n"));
                close(data_listen_sock);
                continue;
            }

            ////////////////// Receive ls result through data connection //////////////////
            total_bytes = 0;

            while ((n = read(data_conn_sock, recv_buff, MAX_BUFF - 1)) > 0)
            {
                recv_buff[n] = '\0';
                total_bytes += n;
                write(STDOUT_FILENO, recv_buff, n);
            }

            close(data_conn_sock);
            close(data_listen_sock);

            ////////////////// Receive 226 reply //////////////////
            memset(recv_buff, 0, MAX_BUFF);
            n = read(control_sock, recv_buff, MAX_BUFF - 1);
            if (n > 0)
            {
                recv_buff[n] = '\0';
                write(STDOUT_FILENO, recv_buff, strlen(recv_buff));
            }

            memset(recv_buff, 0, MAX_BUFF);
            sprintf(recv_buff, "OK. %4d bytes is received.\n", total_bytes);
            write(STDOUT_FILENO, recv_buff, strlen(recv_buff));

            break;
        }
        else
        {
            write(STDOUT_FILENO, "Only ls command is supported.\n",
                  strlen("Only ls command is supported.\n"));
        }
    }

    close(control_sock);

    return 0;
}