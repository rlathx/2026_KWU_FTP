///////////////////////////////////////////////////////////////////////////////
// File Name : srv.c
// Date : 2026/05/01
// OS : Ubuntu 20.04.6 LTS 64bits
// Author : Kim Tae-hyun
// Student ID : 2024402034
// --------------------------------------------------------------------
// Title : System Programming Assignment #2-2 ( FTP Server )
// Description :
// This program receives the port number as an argument and waits for client
// connections. When a client connects, the server creates a child process.
// The parent process prints the client information and waits for another
// client. The child process receives a string from the client and sends the
// same string back to the client. If the received string is "QUIT", the child
// process closes the connection and terminates using SIGALRM.
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define BUF_SIZE 256

void sh_chld(int); // Signal handler for SIGCHLD
void sh_alrm(int); // Signal handler for SIGALRM

///////////////////////////////////////////////////////////////////////////////////////
// main
// ===================================================================
// Input: int argc -> Number of command line arguments
//        char **argv -> Port number
//
// Output: int -> 0 when program terminates normally
//
// Purpose:
// Creates a server socket and waits for client connections.
// When a client connects, the server creates a child process.
// The parent process prints client information, and the child process
// communicates with the connected client.
///////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    char buff[BUF_SIZE];
    int n;
    struct sockaddr_in server_addr, client_addr;
    int server_fd, client_fd;
    socklen_t len;

    ////////////////////////////// Signal handler setting //////////////////////////////
    signal(SIGCHLD, sh_chld);
    signal(SIGALRM, sh_alrm);
    ////////////////////////////// End of signal handler setting //////////////////////////////

    ////////////////////////////// Socket creation //////////////////////////////
    server_fd = socket(PF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }
    ////////////////////////////// End of socket creation //////////////////////////////

    ////////////////////////////// Server address setting //////////////////////////////
    memset(&server_addr, 0, sizeof(server_addr)); // Initialize server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));
    ////////////////////////////// End of server address setting //////////////////////////////

    ////////////////////////////// Bind socket //////////////////////////////
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }
    ////////////////////////////// End of bind socket //////////////////////////////

    ////////////////////////////// Listen for client //////////////////////////////
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }
    ////////////////////////////// End of listen for client //////////////////////////////

    while (1)
    {
        pid_t pid;

        len = sizeof(client_addr);

        ////////////////////////////// Accept client //////////////////////////////
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);

        if (client_fd < 0)
            continue;
        ////////////////////////////// End of accept client //////////////////////////////

        ////////////////////////////// Create child process //////////////////////////////
        pid = fork();

        if (pid < 0)
        {
            close(client_fd);
            continue;
        }
        ////////////////////////////// End of create child process //////////////////////////////

        ////////////////////////////// Child process //////////////////////////////
        if (pid == 0)
        {
            close(server_fd); // Child process does not need the listening socket

            while (1)
            {
                memset(buff, 0, BUF_SIZE);

                ////////////////////////////// Receive string from client //////////////////////////////
                n = read(client_fd, buff, BUF_SIZE - 1);

                if (n <= 0)
                    break;

                buff[n] = '\0';
                ////////////////////////////// End of receive string from client //////////////////////////////

                ////////////////////////////// Check QUIT command //////////////////////////////
                if (strncmp(buff, "QUIT", 4) == 0)
                {
                    close(client_fd); // Close connection with the client that sent QUIT
                    raise(SIGALRM);   // Call SIGALRM handler to terminate child process
                }
                ////////////////////////////// End of check QUIT command //////////////////////////////

                ////////////////////////////// Send string to client //////////////////////////////
                if (write(client_fd, buff, n) <= 0)
                    break;
                ////////////////////////////// End of send string to client //////////////////////////////
            }

            close(client_fd);
            exit(0);
        }
        ////////////////////////////// End of child process //////////////////////////////

        ////////////////////////////// Parent process //////////////////////////////
        else
        {
            printf("==========Client info==========\n");
            printf("client IP : %s\n\n", inet_ntoa(client_addr.sin_addr));
            printf("client port : %d\n", ntohs(client_addr.sin_port));
            printf("===============================\n");

            printf("Child Process ID : %d\n", pid);

            close(client_fd); // Parent process does not communicate with this client
        }
        ////////////////////////////// End of parent process //////////////////////////////
    }

    close(server_fd);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// sh_chld
// ===================================================================
// Input: int signum -> Signal number
//
// Output: void
//
// Purpose:
// Handles SIGCHLD signal when a child process terminates.
// It calls wait() to collect the terminated child process and prevent
// zombie processes.
///////////////////////////////////////////////////////////////////////////////////////
void sh_chld(int signum)
{
    printf("Status of Child process was changed.\n");
    wait(NULL);
}

///////////////////////////////////////////////////////////////////////////////////////
// sh_alrm
// ===================================================================
// Input: int signum -> Signal number
//
// Output: void
//
// Purpose:
// Handles SIGALRM signal when the child process receives "QUIT".
// It prints the child process PID and terminates the child process.
///////////////////////////////////////////////////////////////////////////////////////
void sh_alrm(int signum)
{
    printf("Child Process(PID : %d) will be terminated.\n", getpid());
    exit(1);
}