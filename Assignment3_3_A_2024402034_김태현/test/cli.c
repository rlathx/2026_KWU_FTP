///////////////////////////////////////////////////////////////////////////////
// File Name : cli.c
// Date : 2026/06/11
// OS : Ubuntu 20.04.6 LTS 64bits
// Author : Kim Tae Hyeon
// Student ID: 2024402034
// --------------------------------------------------------------------
// Title : System Programming Assignment #3-3 (ftp server)
// Description:
// This program is an FTP client.  It keeps one control connection and opens
// one temporary data connection for ls/get/put by sending PORT to the server.
///////////////////////////////////////////////////////////////////////////////
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define MAX_BUF 4096
#define MAX_CMD 1024
#define MIN_DATA_PORT 10001
#define MAX_DATA_PORT 60000

static int read_line(int fd, char *buf, int size);
static int send_line(int fd, const char *msg);
static void print_line(const char *msg);
static void trim_newline(char *s);
static void read_stdin_line(const char *prompt, char *buf, int size, int hide);
static int parse_code(const char *reply);
static int connect_server(const char *ip, int port);
static int open_data_listener(int *port);
static int send_port_command(int sockfd, int *listenfd);
static void make_port_argument(int sockfd, int port, char *out, int size);
static void command_loop(int sockfd);
static int receive_reply_and_print(int sockfd, char *reply, int size);
static void do_ls(int sockfd, const char *user_cmd, const char *ftp_cmd);
static void do_get(int sockfd, const char *filename);
static void do_put(int sockfd, const char *filename);
static void send_simple_command(int sockfd, const char *cmd);
static void make_ftp_command(char *out, const char *input);
static int starts_with_command(const char *s, const char *cmd);

///////////////////////////////////////////////////////////////////////////////
// main
// =============================================================== //
// Input : argc, argv -> server IP and port
// Output: int - program exit status
// Purpose: Starting client, login, and command loop.
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    int sockfd;
    char reply[MAX_BUF];
    char user[128], passwd[128], sendbuf[MAX_CMD];
    int fail_count = 0;

    if (argc != 3) {
        write(STDOUT_FILENO, "usage: ./cli <IP> <PORT>\n", 25);
        return 1;
    }

    sockfd = connect_server(argv[1], atoi(argv[2]));
    if (sockfd < 0) {
        perror("connect");
        return 1;
    }

    printf("Connected to sswlab.kw.ac.kr.\n");
    if (receive_reply_and_print(sockfd, reply, sizeof(reply)) <= 0) {
        close(sockfd);
        return 1;
    }
    if (parse_code(reply) != 220) {
        close(sockfd);
        return 0;
    }

    while (fail_count < 3) {
        read_stdin_line("Name : ", user, sizeof(user), 0);
        snprintf(sendbuf, sizeof(sendbuf), "USER %s", user);
        send_line(sockfd, sendbuf);
        if (receive_reply_and_print(sockfd, reply, sizeof(reply)) <= 0) break;

        if (parse_code(reply) == 331) {
            read_stdin_line("Password:", passwd, sizeof(passwd), 1);
            snprintf(sendbuf, sizeof(sendbuf), "PASS %s", passwd);
            send_line(sockfd, sendbuf);
            if (receive_reply_and_print(sockfd, reply, sizeof(reply)) <= 0) break;

            if (parse_code(reply) == 230) {
                command_loop(sockfd);
                break;
            }
            fail_count++;
            if (parse_code(reply) == 530) break;
        } else {
            fail_count++;
            if (parse_code(reply) == 530) break;
        }
    }

    close(sockfd);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// connect_server
// =============================================================== //
// Input : ip -> server IP, port -> server port
// Output: int - connected socket fd or -1
// Purpose: Connecting control socket to server.
///////////////////////////////////////////////////////////////////////////////
static int connect_server(const char *ip, int port)
{
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

///////////////////////////////////////////////////////////////////////////////
// command_loop
// =============================================================== //
// Input : sockfd -> control connection socket
// Output: void
// Purpose: Reading user commands and executing FTP requests.
///////////////////////////////////////////////////////////////////////////////
static void command_loop(int sockfd)
{
    char input[MAX_CMD], ftp_cmd[MAX_CMD];

    while (1) {
        write(STDOUT_FILENO, "ftp> ", 5);
        if (read_line(STDIN_FILENO, input, sizeof(input)) <= 0) break;
        trim_newline(input);
        if (strlen(input) == 0) continue;

        make_ftp_command(ftp_cmd, input);

        if (starts_with_command(input, "ls") || starts_with_command(input, "dir")) {
            do_ls(sockfd, input, ftp_cmd);
        } else if (starts_with_command(input, "get")) {
            char *p = input + 3;
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == 0) continue;
            do_get(sockfd, p);
        } else if (starts_with_command(input, "put")) {
            char *p = input + 3;
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == 0) continue;
            do_put(sockfd, p);
        } else {
            send_simple_command(sockfd, ftp_cmd);
            if (starts_with_command(input, "quit") || starts_with_command(input, "bye")) break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// make_ftp_command
// =============================================================== //
// Input : out -> FTP command buffer, input -> user command
// Output: void
// Purpose: Converting user command to FTP command.
///////////////////////////////////////////////////////////////////////////////
static void make_ftp_command(char *out, const char *input)
{
    char cmd[MAX_CMD];
    char arg[MAX_CMD];
    char oldn[MAX_CMD];
    char newn[MAX_CMD];
    const char *p;
    int n;

    cmd[0] = '\0';
    arg[0] = '\0';
    oldn[0] = '\0';
    newn[0] = '\0';

    sscanf(input, "%1023s", cmd);
    p = input + strlen(cmd);

    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }

    strncpy(arg, p, sizeof(arg) - 1);
    arg[sizeof(arg) - 1] = '\0';

    for (n = 0; cmd[n] != '\0'; n++) {
        cmd[n] = (char)tolower((unsigned char)cmd[n]);
    }

    if (strcmp(cmd, "ls") == 0) {
        if (arg[0] == '\0') snprintf(out, MAX_CMD, "NLST");
        else snprintf(out, MAX_CMD, "NLST %s", arg);
    }
    else if (strcmp(cmd, "dir") == 0) {
        if (arg[0] == '\0') snprintf(out, MAX_CMD, "LIST");
        else snprintf(out, MAX_CMD, "LIST %s", arg);
    }
    else if (strcmp(cmd, "pwd") == 0) {
        snprintf(out, MAX_CMD, "PWD");
    }
    else if (strcmp(cmd, "cd") == 0) {
        if (strcmp(arg, "..") == 0) {
            snprintf(out, MAX_CMD, "CDUP");
        }
        else {
            if (arg[0] == '\0') snprintf(out, MAX_CMD, "CWD");
            else snprintf(out, MAX_CMD, "CWD %s", arg);
        }
    }
    else if (strcmp(cmd, "mkdir") == 0) {
        if (arg[0] == '\0') snprintf(out, MAX_CMD, "MKD");
        else snprintf(out, MAX_CMD, "MKD %s", arg);
    }
    else if (strcmp(cmd, "delete") == 0) {
        if (arg[0] == '\0') snprintf(out, MAX_CMD, "DELE");
        else snprintf(out, MAX_CMD, "DELE %s", arg);
    }
    else if (strcmp(cmd, "rmdir") == 0) {
        if (arg[0] == '\0') snprintf(out, MAX_CMD, "RMD");
        else snprintf(out, MAX_CMD, "RMD %s", arg);
    }
    else if (strcmp(cmd, "rename") == 0) {
        sscanf(arg, "%1023s %1023s", oldn, newn);
        snprintf(out, MAX_CMD, "RNFR %s RNTO %s", oldn, newn);
    }
    else if (strcmp(cmd, "type") == 0) {
        if (arg[0] == '\0') snprintf(out, MAX_CMD, "TYPE");
        else snprintf(out, MAX_CMD, "TYPE %s", arg);
    }
    else if (strcmp(cmd, "bin") == 0) {
        snprintf(out, MAX_CMD, "TYPE I");
    }
    else if (strcmp(cmd, "ascii") == 0) {
        snprintf(out, MAX_CMD, "TYPE A");
    }
    else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "bye") == 0) {
        snprintf(out, MAX_CMD, "QUIT");
    }
    else {
        snprintf(out, MAX_CMD, "%s", input);
    }
}

///////////////////////////////////////////////////////////////////////////////
// send_simple_command
// =============================================================== //
// Input : sockfd -> control socket, cmd -> FTP command
// Output: void
// Purpose: Sending command and printing server reply.
///////////////////////////////////////////////////////////////////////////////
static void send_simple_command(int sockfd, const char *cmd)
{
    char reply[MAX_BUF];

    send_line(sockfd, cmd);
    if (receive_reply_and_print(sockfd, reply, sizeof(reply)) <= 0) return;

    if (starts_with_command(cmd, "RNFR") && parse_code(reply) == 350) {
        receive_reply_and_print(sockfd, reply, sizeof(reply));
    }
}

///////////////////////////////////////////////////////////////////////////////
// do_ls
// =============================================================== //
// Input : sockfd -> control socket, user_cmd -> original command, ftp_cmd -> FTP command
// Output: void
// Purpose: Receiving directory list through data connection.
///////////////////////////////////////////////////////////////////////////////
static void do_ls(int sockfd, const char *user_cmd, const char *ftp_cmd)
{
    int listenfd, datafd, n, total = 0;
    char buf[MAX_BUF], reply[MAX_BUF];
    (void)user_cmd;

    if (send_port_command(sockfd, &listenfd) < 0) return;
    send_line(sockfd, ftp_cmd);
    if (receive_reply_and_print(sockfd, reply, sizeof(reply)) <= 0) { close(listenfd); return; }
    if (parse_code(reply) != 150) { close(listenfd); return; }

    datafd = accept(listenfd, NULL, NULL);
    if (datafd >= 0) {
        while ((n = read(datafd, buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, n);
            total += n;
        }
        close(datafd);
    }
    close(listenfd);
    if (receive_reply_and_print(sockfd, reply, sizeof(reply)) > 0)
        printf("OK. %d bytes is received.\n", total);
}

///////////////////////////////////////////////////////////////////////////////
// do_get
// =============================================================== //
// Input : sockfd -> control socket, filename -> remote file name
// Output: void
// Purpose: Downloading file from server.
///////////////////////////////////////////////////////////////////////////////
static void do_get(int sockfd, const char *filename)
{
    int listenfd, datafd, fd, n, total = 0;
    char buf[MAX_BUF], reply[MAX_BUF], cmd[MAX_CMD];
    char localname[MAX_CMD];
    char *base;
    char *save_name;

    if (send_port_command(sockfd, &listenfd) < 0) return;
    snprintf(cmd, sizeof(cmd), "RETR %s", filename);
    send_line(sockfd, cmd);
    if (receive_reply_and_print(sockfd, reply, sizeof(reply)) <= 0) { close(listenfd); return; }
    if (parse_code(reply) != 150) { close(listenfd); return; }

    strncpy(localname, filename, sizeof(localname) - 1);
    localname[sizeof(localname) - 1] = '\0';
    base = strrchr(localname, '/');
    if (base == NULL) save_name = localname;
    else save_name = base + 1;
    fd = open(save_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    datafd = accept(listenfd, NULL, NULL);
    if (datafd >= 0 && fd >= 0) {
        while ((n = read(datafd, buf, sizeof(buf))) > 0) {
            write(fd, buf, n);
            total += n;
        }
    }
    if (fd >= 0) close(fd);
    if (datafd >= 0) close(datafd);
    close(listenfd);
    if (receive_reply_and_print(sockfd, reply, sizeof(reply)) > 0)
        printf("OK. %d bytes is received.\n", total);
}

///////////////////////////////////////////////////////////////////////////////
// do_put
// =============================================================== //
// Input : sockfd -> control socket, filename -> local file name
// Output: void
// Purpose: Uploading file to server.
///////////////////////////////////////////////////////////////////////////////
static void do_put(int sockfd, const char *filename)
{
    int listenfd, datafd, fd, n, total = 0;
    char buf[MAX_BUF], reply[MAX_BUF], cmd[MAX_CMD];

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("550 %s: No such file or directory.\n", filename);
        return;
    }

    if (send_port_command(sockfd, &listenfd) < 0) { close(fd); return; }
    snprintf(cmd, sizeof(cmd), "STOR %s", filename);
    send_line(sockfd, cmd);
    if (receive_reply_and_print(sockfd, reply, sizeof(reply)) <= 0) { close(fd); close(listenfd); return; }
    if (parse_code(reply) != 150) { close(fd); close(listenfd); return; }

    datafd = accept(listenfd, NULL, NULL);
    if (datafd >= 0) {
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            write(datafd, buf, n);
            total += n;
        }
        close(datafd);
    }
    close(fd);
    close(listenfd);
    if (receive_reply_and_print(sockfd, reply, sizeof(reply)) > 0)
        printf("OK. %d bytes is sent.\n", total);
}

///////////////////////////////////////////////////////////////////////////////
// send_port_command
// =============================================================== //
// Input : sockfd -> control socket, listenfd -> data listen socket
// Output: int - 0 success, -1 fail
// Purpose: Opening data port and sending PORT command.
///////////////////////////////////////////////////////////////////////////////
static int send_port_command(int sockfd, int *listenfd)
{
    int port;
    char cmd[MAX_CMD], reply[MAX_BUF], port_arg[MAX_CMD];

    *listenfd = open_data_listener(&port);
    if (*listenfd < 0) return -1;

    make_port_argument(sockfd, port, port_arg, sizeof(port_arg));
    snprintf(cmd, sizeof(cmd), "PORT %s", port_arg);
    send_line(sockfd, cmd);
    if (receive_reply_and_print(sockfd, reply, sizeof(reply)) <= 0) {
        close(*listenfd);
        return -1;
    }
    if (parse_code(reply) != 200) {
        close(*listenfd);
        return -1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// make_port_argument
// =============================================================== //
// Input : sockfd -> control socket, port -> data port, out -> PORT argument
// Output: void
// Purpose: Making h1,h2,h3,h4,p1,p2 string.
///////////////////////////////////////////////////////////////////////////////
static void make_port_argument(int sockfd, int port, char *out, int size)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    unsigned char *ip;

    memset(&addr, 0, sizeof(addr));
    if (getsockname(sockfd, (struct sockaddr *)&addr, &len) < 0) {
        snprintf(out, size, "127,0,0,1,%d,%d", port / 256, port % 256);
        return;
    }

    ip = (unsigned char *)&addr.sin_addr.s_addr;
    snprintf(out, size, "%u,%u,%u,%u,%d,%d",
             ip[0], ip[1], ip[2], ip[3], port / 256, port % 256);
}

///////////////////////////////////////////////////////////////////////////////
// open_data_listener
// =============================================================== //
// Input : port -> selected data port
// Output: int - listen socket fd or -1
// Purpose: Creating client-side data listening socket.
///////////////////////////////////////////////////////////////////////////////
static int open_data_listener(int *port)
{
    int listenfd, opt = 1, i;
    struct sockaddr_in addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) return -1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    srand((unsigned int)(time(NULL) ^ getpid()));
    for (i = 0; i < 1000; i++) {
        *port = MIN_DATA_PORT + rand() % (MAX_DATA_PORT - MIN_DATA_PORT + 1);
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(*port);
        if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            listen(listenfd, 1);
            return listenfd;
        }
    }
    close(listenfd);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
// receive_reply_and_print
// =============================================================== //
// Input : sockfd -> control socket, reply -> reply buffer
// Output: int - read byte count
// Purpose: Reading and printing server reply.
///////////////////////////////////////////////////////////////////////////////
static int receive_reply_and_print(int sockfd, char *reply, int size)
{
    int n = read_line(sockfd, reply, size);
    if (n > 0) print_line(reply);
    return n;
}

///////////////////////////////////////////////////////////////////////////////
// read_line
// =============================================================== //
// Input : fd -> file descriptor, buf -> input buffer, size -> buffer size
// Output: int - read byte count
// Purpose: Reading one line from descriptor.
///////////////////////////////////////////////////////////////////////////////
static int read_line(int fd, char *buf, int size)
{
    int i = 0;
    char c;
    while (i < size - 1) {
        int n = read(fd, &c, 1);
        if (n <= 0) {
            if (i == 0) return n;
            break;
        }
        if (c == '\r') continue;
        buf[i++] = c;
        if (c == '\n') break;
    }
    buf[i] = '\0';
    trim_newline(buf);
    return i;
}

///////////////////////////////////////////////////////////////////////////////
// send_line
// =============================================================== //
// Input : fd -> file descriptor, msg -> message string
// Output: int - written byte count
// Purpose: Sending one line with newline.
///////////////////////////////////////////////////////////////////////////////
static int send_line(int fd, const char *msg)
{
    char buf[MAX_CMD + 4];
    snprintf(buf, sizeof(buf), "%s\n", msg);
    return write(fd, buf, strlen(buf));
}

///////////////////////////////////////////////////////////////////////////////
// print_line
// =============================================================== //
// Input : msg -> output message
// Output: void
// Purpose: Printing one line to stdout.
///////////////////////////////////////////////////////////////////////////////
static void print_line(const char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
    write(STDOUT_FILENO, "\n", 1);
}

///////////////////////////////////////////////////////////////////////////////
// trim_newline
// =============================================================== //
// Input : s -> target string
// Output: void
// Purpose: Removing CR/LF characters.
///////////////////////////////////////////////////////////////////////////////
static void trim_newline(char *s)
{
    s[strcspn(s, "\r\n")] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
// parse_code
// =============================================================== //
// Input : reply -> server reply string
// Output: int - FTP reply code or 0
// Purpose: Parsing 3-digit FTP reply code.
///////////////////////////////////////////////////////////////////////////////
static int parse_code(const char *reply)
{
    int code;

    if (!isdigit((unsigned char)reply[0])) return 0;
    if (!isdigit((unsigned char)reply[1])) return 0;
    if (!isdigit((unsigned char)reply[2])) return 0;

    code = (reply[0] - '0') * 100;
    code = code + (reply[1] - '0') * 10;
    code = code + (reply[2] - '0');

    return code;
}

///////////////////////////////////////////////////////////////////////////////
// read_stdin_line
// =============================================================== //
// Input : prompt -> prompt text, buf -> input buffer, hide -> echo flag
// Output: void
// Purpose: Reading user input from stdin.
///////////////////////////////////////////////////////////////////////////////
static void read_stdin_line(const char *prompt, char *buf, int size, int hide)
{
    struct termios oldt, newt;
    write(STDOUT_FILENO, prompt, strlen(prompt));
    if (hide) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }
    read_line(STDIN_FILENO, buf, size);
    if (hide) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        write(STDOUT_FILENO, "\n", 1);
    }
}

///////////////////////////////////////////////////////////////////////////////
// starts_with_command
// =============================================================== //
// Input : s -> input string, cmd -> command keyword
// Output: int - 1 match, 0 not match
// Purpose: Checking command prefix.
///////////////////////////////////////////////////////////////////////////////
static int starts_with_command(const char *s, const char *cmd)
{
    int len;

    len = strlen(cmd);
    if (strncasecmp(s, cmd, len) != 0) return 0;
    if (s[len] == '\0') return 1;
    if (isspace((unsigned char)s[len])) return 1;

    return 0;
}
