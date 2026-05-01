///////////////////////////////////////////////////////////////////////////////
// File Name : cli.c
// Date : 2026/05/01
// OS : Ubuntu 20.04.6 LTS 64bits
// Author : Kim Tae-hyun
// Student ID : 2024402034
// --------------------------------------------------------------------
// Title : System Programming Assignment #2-2 ( FTP Server )
// Description :
// This program receives the server IP address and port number as arguments.
// After connecting to the server, it repeatedly receives a string from the
// user, sends the string to the server, receives the echoed string from the
// server, and prints it to standard output.
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define BUF_SIZE 256

///////////////////////////////////////////////////////////////////////////////////////
// main
// ===================================================================
// Input: int argc -> Number of command line arguments
//        char **argv -> Server IP address and port number
//
// Output: int -> 0 when program terminates normally
//
// Purpose:
// Connects to the server using the given IP address and port number.
// Repeatedly reads a string from standard input, sends it to the server,
// receives the echoed string from the server, and prints it.
///////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    char buff[BUF_SIZE];
    int n;
    int sockfd;
    struct sockaddr_in serv_addr;

    ////////////////////////////// Socket creation //////////////////////////////
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }
    ////////////////////////////// End of socket creation //////////////////////////////

    ////////////////////////////// Server address setting //////////////////////////////
    memset(&serv_addr, 0, sizeof(serv_addr)); // Initialize server address structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    ////////////////////////////// End of server address setting //////////////////////////////

    ////////////////////////////// Server connection //////////////////////////////
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(sockfd);
        return 1;
    }
    ////////////////////////////// End of server connection //////////////////////////////

    while (1)
    {
        memset(buff, 0, BUF_SIZE);

        ////////////////////////////// User input //////////////////////////////
        write(STDOUT_FILENO, "> ", 2);

        n = read(STDIN_FILENO, buff, BUF_SIZE - 1);

        if (n <= 0)
            break;

        buff[n] = '\0';
        ////////////////////////////// End of user input //////////////////////////////

        ////////////////////////////// Send string to server //////////////////////////////
        if (write(sockfd, buff, n) > 0)
        {
            memset(buff, 0, BUF_SIZE);

            ////////////////////////////// Receive string from server //////////////////////////////
            n = read(sockfd, buff, BUF_SIZE - 1);

            if (n > 0)
            {
                buff[n] = '\0';
                printf("from server: %s", buff);
            }
            else
                break;
            ////////////////////////////// End of receive string from server //////////////////////////////
        }
        else
            break;
        ////////////////////////////// End of send string to server //////////////////////////////
    }

    ////////////////////////////// Close socket //////////////////////////////
    close(sockfd);
    ////////////////////////////// End of close socket //////////////////////////////

    return 0;
}