///////////////////////////////////////////////////////////////////////////////
// File Name : srv.c
// Date      : 2026/05/19
// OS        : Ubuntu 20.04.6 LTS 64bits
// Author    : Kim Tae Hyun
// Student ID: 2024402034
// --------------------------------------------------------------------
// Title     : System Programming Assignment #3-1 ( ftp server )
// Description:
// This program is a server program for FTP Assignment 3-1.
// The server waits for client connection using the given port number.
// After accepting a client, it checks whether the client's IP exists
// in access.txt. If the client is accepted, the server receives
// username and password, compares them with the passwd file,
// and sends login result to the client.
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_BUF 20

int log_auth(int connfd);
int user_match(char *user, char *passwd);
int check_ip(char *client_ip);

///////////////////////////////////////////////////////////////////////////////////////
// main
// ===================================================================
// Input : int argc    -> argument count
//         char *argv[] -> argv[1] is server port number
//
// Output: int -> 0 when program terminates normally
//
// Purpose:
// Creates server socket, binds it to the given port number,
// waits for client connection, checks client's IP,
// and performs login authentication.
///////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    FILE *fp_checkIP;

    ////////////////////////// Create server socket //////////////////////////
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    //////////////////////// End of creating server socket ///////////////////

    ////////////////////////// Set server address information ////////////////
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
    //////////////////////// End of setting server address ///////////////////

    ////////////////////////// Bind socket to server address //////////////////
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    //////////////////////// End of binding socket ///////////////////////////

    listen(listenfd, 5);

    for (;;)
    {
        ////////////////////////// Accept client connection ///////////////////
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        //////////////////////// End of accepting client //////////////////////

        ////////////////////////// Display client information //////////////////
        printf("** Client is trying to connect **\n");
        printf("- IP: %s\n", inet_ntoa(cliaddr.sin_addr));
        printf("- Port: %d\n", ntohs(cliaddr.sin_port));
        //////////////////////// End of displaying client information /////////

        ////////////////////////// Check access.txt existence //////////////////
        fp_checkIP = fopen("access.txt", "r");

        if (fp_checkIP == NULL)
        {
            write(connfd, "REJECTION", MAX_BUF);
            printf("** It is NOT authenticated client **\n");
            close(connfd);
            continue;
        }

        fclose(fp_checkIP);
        //////////////////////// End of checking access.txt existence /////////

        ////////////////////////// Check client IP authentication //////////////
        if (check_ip(inet_ntoa(cliaddr.sin_addr)) == 0)
        {
            write(connfd, "REJECTION", MAX_BUF);
            printf("** It is NOT authenticated client **\n");
            close(connfd);
            continue;
        }

        write(connfd, "ACCEPTED", MAX_BUF);
        printf("** Client is connected **\n");
        //////////////////////// End of client IP authentication //////////////

        ////////////////////////// Start login authentication //////////////////
        if (log_auth(connfd) == 0)
        {
            printf("** Fail to log-in **\n");
            close(connfd);
            continue;
        }

        printf("** Success to log-in **\n");
        close(connfd);
        //////////////////////// End of login authentication //////////////////
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// log_auth
// ===================================================================
// Input : int connfd -> connected socket file descriptor
//
// Output: int -> 1 when login succeeds
//               0 when login fails three times
//
// Purpose:
// Receives username and password from client.
// Sends "OK" after receiving login information.
// Compares received username and password using user_match().
// Sends "OK", "FAIL", or "DISCONNECTION" according to login result.
///////////////////////////////////////////////////////////////////////////////////////
int log_auth(int connfd)
{
    char user[MAX_BUF], passwd[MAX_BUF];
    int n, count = 1;

    while (1)
    {
        ////////////////////////// Initialize login buffers ////////////////////
        memset(user, 0, MAX_BUF);
        memset(passwd, 0, MAX_BUF);
        //////////////////////// End of initializing login buffers ////////////

        ////////////////////////// Receive username and password ///////////////
        printf("** User is trying to log-in (%d/3) **\n", count);

        read(connfd, user, MAX_BUF);
        read(connfd, passwd, MAX_BUF);
        //////////////////////// End of receiving username and password ///////

        ////////////////////////// Send receive confirmation ///////////////////
        write(connfd, "OK", MAX_BUF);
        //////////////////////// End of sending receive confirmation //////////

        ////////////////////////// Check user authentication ///////////////////
        if ((n = user_match(user, passwd)) == 1)
        {
            write(connfd, "OK", MAX_BUF);
            break;
        }
        else if (n == 0)
        {
            printf("** Log-in failed **\n");

            if (count >= 3)
            {
                write(connfd, "DISCONNECTION", MAX_BUF);
                return 0;
            }

            write(connfd, "FAIL", MAX_BUF);
            count++;
            continue;
        }
        //////////////////////// End of checking user authentication //////////
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////
// user_match
// ===================================================================
// Input : char *user   -> username received from client
//         char *passwd -> password received from client
//
// Output: int -> 1 when username and password are found in passwd file
//               0 when username and password are not found
//
// Purpose:
// Opens passwd file and searches whether received username and password
// match one line of the passwd file.
///////////////////////////////////////////////////////////////////////////////////////
int user_match(char *user, char *passwd)
{
    FILE *fp;
    char line[100];
    char file_user[MAX_BUF];
    char file_passwd[MAX_BUF];

    ////////////////////////// Open passwd file ///////////////////////////////
    fp = fopen("passwd.txt", "r");
    //////////////////////// End of opening passwd file ///////////////////////

    ////////////////////////// Search username and password ///////////////////
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        sscanf(line, "%19[^:]:%19[^:]", file_user, file_passwd);

        if (!strcmp(user, file_user) && !strcmp(passwd, file_passwd))
        {
            fclose(fp);
            return 1;
        }
    }
    //////////////////////// End of searching username and password ///////////

    fclose(fp);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// check_ip
// ===================================================================
// Input : char *client_ip -> client IP address
//
// Output: int -> 1 when client IP exists in access.txt
//               0 when client IP does not exist in access.txt
//
// Purpose:
// Opens access.txt and checks whether the client's IP address is allowed.
// If access.txt contains "*.*.*.*", every client IP is accepted.
///////////////////////////////////////////////////////////////////////////////////////
int check_ip(char *client_ip)
{
    FILE *fp;
    char ip[100];

    ////////////////////////// Open access.txt ////////////////////////////////
    fp = fopen("access.txt", "r");
    //////////////////////// End of opening access.txt ////////////////////////

    ////////////////////////// Search client IP ///////////////////////////////
    while (fgets(ip, sizeof(ip), fp) != NULL)
    {
        ip[strcspn(ip, "\n")] = '\0';

        if (!strcmp(ip, client_ip) || !strcmp(ip, "*.*.*.*"))
        {
            fclose(fp);
            return 1;
        }
    }
    //////////////////////// End of searching client IP ///////////////////////

    fclose(fp);
    return 0;
}