///////////////////////////////////////////////////////////////////////////////
// File Name : cli.c
// Date      : 2026/05/19
// OS        : Ubuntu 20.04.6 LTS 64bits
// Author    : Kim Tae Hyeon
// Student ID: 2024402034
// --------------------------------------------------------------------
// Title     : System Programming Assignment #3-1 ( ftp server )
// Description:
// This program is a client program for FTP Assignment 3-1.
// The client connects to the server using IP address and port number.
// After connection, it receives access result from server.
// If the connection is accepted, it receives username and password
// from standard input and sends them to the server.
// Then it prints login result according to the server response.
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_BUF 20
#define CONT_PORT 20001

void log_in(int sockfd);

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in servaddr;

    ////////////////////////// Create client socket //////////////////////////
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //////////////////////// End of creating client socket ///////////////////

    ////////////////////////// Set server address information //////////////////
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));
    //////////////////////// End of setting server address ////////////////////

    ////////////////////////// Connect to server //////////////////////////////
    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    //////////////////////// End of connecting to server //////////////////////

    log_in(sockfd);

    close(sockfd);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// log_in
// ===================================================================
// Input : int sockfd -> socket file descriptor connected to server
//
// Output: void
//
// Purpose:
// Checks whether the client is accepted by server.
// If accepted, receives username and password from standard input,
// sends them to server, and prints login result according to server response.
///////////////////////////////////////////////////////////////////////////////////////
void log_in(int sockfd)
{
    int n;
    char user[MAX_BUF], passwd[MAX_BUF], buf[MAX_BUF];

    ////////////////////////// Check if client IP is acceptable /////////////////////////
    n = read(sockfd, buf, MAX_BUF);
    buf[n] = '\0';

    if (!strcmp(buf, "REJECTION"))
    {
        printf("** Connection refused **\n");
        return;
    }
    else if (!strcmp(buf, "ACCEPTED"))
    {
        printf("** It is connected to Server **\n");
    }
    //////////////////////// End of client IP authentication ///////////////////////////

    for (;;)
    {
        ////////////////////////// Pass username and password to server /////////////////
        printf("Input ID : ");
        fflush(stdout);

        n = read(STDIN_FILENO, user, MAX_BUF);
        user[n - 1] = '\0';     // remove newline character

        printf("Input Password : ");
        fflush(stdout);

        n = read(STDIN_FILENO, passwd, MAX_BUF);
        passwd[n - 1] = '\0';   // remove newline character

        write(sockfd, user, MAX_BUF);
        write(sockfd, passwd, MAX_BUF);
        //////////////////////// End of passing login information ///////////////////////

        ////////////////////////// Receive login process response ///////////////////////
        n = read(sockfd, buf, MAX_BUF);
        buf[n] = '\0';

        if (!strcmp(buf, "OK")) // OK after server receives username and password
        {
            ////////////////////////// Receive final login result ////////////////////////
            n = read(sockfd, buf, MAX_BUF);
            buf[n] = '\0';

            if (!strcmp(buf, "OK"))
            {
                printf("** User '%s' logged in **\n", user);
                break;
            }
            else if (!strcmp(buf, "FAIL"))
            {
                printf("** Log-in failed **\n");
            }
            else if (!strcmp(buf, "DISCONNECTION"))
            {
                printf("** Connection closed **\n");
                break;
            }
            //////////////////////// End of final login result //////////////////////////
        }
        //////////////////////// End of login process response //////////////////////////
    }
}