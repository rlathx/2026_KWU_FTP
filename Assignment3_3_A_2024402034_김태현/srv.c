#define _DEFAULT_SOURCE
///////////////////////////////////////////////////////////////////////////////
// File Name : srv.c
// Date : 2026/06/11
// OS : Ubuntu 20.04.6 LTS 64bits
// Author : Kim Tae Hyeon
// Student ID: 2024402034
// --------------------------------------------------------------------
// Title : System Programming Assignment #3-3 (ftp server)
// Description:
// This program is an FTP server.  It authenticates clients using access.txt
// and passwd/passwd.txt, processes FTP commands, and transfers data through
// an active-mode data connection opened by PORT.
///////////////////////////////////////////////////////////////////////////////
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_BUF 4096
#define MAX_CMD 1024
#define PATH_BUF 4096

static volatile sig_atomic_t g_listenfd = -1;
static FILE *g_logfp = NULL;

typedef struct ClientInfo {
    char ip[64];
    int port;
    char user[128];
    int data_port;
    char data_ip[64];
    int type;
} ClientInfo;

static int read_line(int fd, char *buf, int size);
static int send_reply(int fd, ClientInfo *cli, int code, const char *fmt, ...);
static void trim_newline(char *s);
static int parse_code_line(const char *line, char *cmd, char *arg);
static int check_ip(const char *client_ip);
static int user_exists(const char *user);
static int user_match(const char *user, const char *passwd);
static void handle_client(int connfd, struct sockaddr_in *cliaddr);
static int login_process(int connfd, ClientInfo *cli);
static void command_process(int connfd, ClientInfo *cli);
static int connect_data(ClientInfo *cli);
static int parse_port_argument(const char *arg, ClientInfo *cli);
static void make_mode_string(ClientInfo *cli, char *out, int size);
static int split_rename_arg(const char *arg, char *oldn, char *newn);
static void reply_motd(int connfd, ClientInfo *cli);
static void log_open(void);
static void write_log(ClientInfo *cli, const char *fmt, ...);
static void sigint_handler(int signo);
static void sigchld_handler(int signo);
static void uppercase(char *s);

static int list_to_fd(int fd, const char *arg, int list_all, int long_format);
static int list_one(int fd, const char *path, const char *name, int long_format);
static void mode_to_string(mode_t mode, char *out);
static void time_to_string(time_t t, char *out, int size);

///////////////////////////////////////////////////////////////////////////////
// main
// =============================================================== //
// Input : argc, argv -> server port
// Output: int - program exit status
// Purpose: Starting server socket and accepting clients.
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    int listenfd, opt = 1;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;

    if (argc != 2) {
        write(STDOUT_FILENO, "usage: ./srv <PORT>\n", 20);
        return 1;
    }

    log_open();
    write_log(NULL, "Server is started");

    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        return 1;
    }
    g_listenfd = listenfd;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        return 1;
    }
    listen(listenfd, 5);

    while (1) {
        int connfd;
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        if (fork() == 0) {
            close(listenfd);
            handle_client(connfd, &cliaddr);
            close(connfd);
            exit(0);
        }
        close(connfd);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// handle_client
// =============================================================== //
// Input : connfd -> client socket, cliaddr -> client address
// Output: void
// Purpose: Checking access, sending welcome, and handling client.
///////////////////////////////////////////////////////////////////////////////
static void handle_client(int connfd, struct sockaddr_in *cliaddr)
{
    ClientInfo cli;
    memset(&cli, 0, sizeof(cli));
    strncpy(cli.ip, inet_ntoa(cliaddr->sin_addr), sizeof(cli.ip) - 1);
    strncpy(cli.data_ip, cli.ip, sizeof(cli.data_ip) - 1);
    cli.port = ntohs(cliaddr->sin_port);
    cli.data_port = -1;
    cli.type = 0;

    printf("** Client is trying to connect **\n");
    printf("- IP: %s\n", cli.ip);
    printf("- Port: %d\n", cli.port);

    if (!check_ip(cli.ip)) {
        send_reply(connfd, &cli, 431, "This client can't access. Close the session.");
        write_log(&cli, "Illegal user connects to server");
        return;
    }

    reply_motd(connfd, &cli);

    if (!login_process(connfd, &cli)) {
        return;
    }
    command_process(connfd, &cli);
}

///////////////////////////////////////////////////////////////////////////////
// login_process
// =============================================================== //
// Input : connfd -> client socket, cli -> client info
// Output: int - 1 success, 0 fail
// Purpose: Authenticating USER and PASS commands.
///////////////////////////////////////////////////////////////////////////////
static int login_process(int connfd, ClientInfo *cli)
{
    char line[MAX_CMD], cmd[64], arg[MAX_CMD];
    char username[128];
    int count = 0;

    memset(username, 0, sizeof(username));
    while (count < 3) {
        if (read_line(connfd, line, sizeof(line)) <= 0) return 0;
        parse_code_line(line, cmd, arg);
        write_log(cli, "Client command: %s", line);

        if (strcmp(cmd, "USER") != 0) {
            send_reply(connfd, cli, 430, "Invalid username or password");
            count++;
            continue;
        }

        strncpy(username, arg, sizeof(username) - 1);
        if (!user_exists(username)) {
            count++;
            if (count >= 3) {
                send_reply(connfd, cli, 530, "Failed to log-in");
                return 0;
            }
            send_reply(connfd, cli, 430, "Invalid username or password");
            continue;
        }
        send_reply(connfd, cli, 331, "Password required for %s.", username);

        if (read_line(connfd, line, sizeof(line)) <= 0) return 0;
        parse_code_line(line, cmd, arg);
        write_log(cli, "Client command: PASS ********");

        if (strcmp(cmd, "PASS") == 0 && user_match(username, arg)) {
            strncpy(cli->user, username, sizeof(cli->user) - 1);
            send_reply(connfd, cli, 230, "User %s logged in.", username);
            write_log(cli, "Authenticated-user connects to server");
            return 1;
        }

        count++;
        if (count >= 3) {
            send_reply(connfd, cli, 530, "Failed to log-in");
            return 0;
        }
        send_reply(connfd, cli, 430, "Invalid username or password");
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// command_process
// =============================================================== //
// Input : connfd -> client socket, cli -> client info
// Output: void
// Purpose: Processing FTP commands after login.
///////////////////////////////////////////////////////////////////////////////
static void command_process(int connfd, ClientInfo *cli)
{
    char line[MAX_CMD], cmd[64], arg[MAX_CMD];
    char oldn[MAX_CMD], newn[MAX_CMD], cwd[PATH_BUF], mode[32];
    int datafd, fd, n, total;
    char buf[MAX_BUF];

    while (1) {
        if (read_line(connfd, line, sizeof(line)) <= 0) {
            write_log(cli, "Client disconnected");
            return;
        }
        parse_code_line(line, cmd, arg);
        write_log(cli, "Client command: %s", line);

        if (!strcmp(cmd, "PORT")) {
            if (parse_port_argument(arg, cli) == 0)
                send_reply(connfd, cli, 200, "PORT command performed successfully.");
            else
                send_reply(connfd, cli, 550, "Failed to access.");
        }
        else if (!strcmp(cmd, "PWD")) {
            getcwd(cwd, sizeof(cwd));
            send_reply(connfd, cli, 257, "\"%s\" is current directory.", cwd);
        }
        else if (!strcmp(cmd, "CWD")) {
            if (chdir(arg) == 0) send_reply(connfd, cli, 250, "CWD command succeeds.");
            else send_reply(connfd, cli, 550, "%s: Can't find such file or directory.", arg);
        }
        else if (!strcmp(cmd, "CDUP")) {
            if (chdir("..") == 0) send_reply(connfd, cli, 250, "CWD command performed successfully.");
            else send_reply(connfd, cli, 550, "..: Can't find such file or directory.");
        }
        else if (!strcmp(cmd, "MKD")) {
            if (mkdir(arg, 0755) == 0) send_reply(connfd, cli, 250, "MKD command performed successfully.");
            else send_reply(connfd, cli, 550, "%s: can't create directory.", arg);
        }
        else if (!strcmp(cmd, "DELE")) {
            if (unlink(arg) == 0) send_reply(connfd, cli, 250, "DELE command performed successfully.");
            else send_reply(connfd, cli, 550, "%s: Can't find such file or directory.", arg);
        }
        else if (!strcmp(cmd, "RMD")) {
            if (rmdir(arg) == 0 || unlink(arg) == 0) send_reply(connfd, cli, 250, "RMD command performed successfully.");
            else send_reply(connfd, cli, 550, "%s: Can't remove directory.", arg);
        }
        else if (!strcmp(cmd, "RNFR")) {
            if (split_rename_arg(arg, oldn, newn) < 0 || access(oldn, F_OK) < 0) {
                send_reply(connfd, cli, 550, "%s: Can't find such file or directory.", oldn[0] ? oldn : arg);
            } else {
                send_reply(connfd, cli, 350, "File exists, ready to rename.");
                if (rename(oldn, newn) == 0) send_reply(connfd, cli, 250, "RNTO command succeeds.");
                else send_reply(connfd, cli, 550, "%s: can't be renamed.", oldn);
            }
        }
        else if (!strcmp(cmd, "TYPE")) {
            if (!strcasecmp(arg, "I") || !strcasecmp(arg, "binary")) {
                cli->type = 0;
                send_reply(connfd, cli, 201, "Type set to I.");
            } else if (!strcasecmp(arg, "A") || !strcasecmp(arg, "ascii")) {
                cli->type = 1;
                send_reply(connfd, cli, 201, "Type set to A.");
            } else {
                send_reply(connfd, cli, 550, "Invalid type.");
            }
        }
        else if (!strcmp(cmd, "NLST") || !strcmp(cmd, "LIST")) {
            int list_all = 0, long_format = 0;
            char target[MAX_CMD] = ".";
            char *tok;

            if (cli->data_port < 0) {
                send_reply(connfd, cli, 550, "Failed to access.");
                continue;
            }
            strncpy(buf, arg, sizeof(buf) - 1); buf[sizeof(buf) - 1] = 0;
            tok = strtok(buf, " ");
            while (tok) {
                if (tok[0] == '-') {
                    if (strchr(tok, 'a')) list_all = 1;
                    if (strchr(tok, 'l')) long_format = 1;
                } else {
                    strncpy(target, tok, sizeof(target) - 1);
                }
                tok = strtok(NULL, " ");
            }
            if (!strcmp(cmd, "LIST")) { list_all = 1; long_format = 1; }

            if (access(target, F_OK) < 0) {
                send_reply(connfd, cli, 550, "%s: No such file or directory.", target);
                continue;
            }

            datafd = connect_data(cli);
            if (datafd < 0) {
                send_reply(connfd, cli, 550, "Failed to access.");
                continue;
            }
            send_reply(connfd, cli, 150, "Opening data connection for directory list.");
            total = list_to_fd(datafd, target, list_all, long_format);
            close(datafd);
            if (total >= 0) {
                send_reply(connfd, cli, 226, "Complete transmission.");
            }
            else {
                send_reply(connfd, cli, 550, "Failed transmission.");
            }
            cli->data_port = -1;
        }
        else if (!strcmp(cmd, "RETR")) {
            if (cli->data_port < 0) {
                send_reply(connfd, cli, 550, "Failed to access.");
                continue;
            }
            fd = open(arg, O_RDONLY);
            if (fd < 0) {
                send_reply(connfd, cli, 550, "%s: No such file or directory.", arg);
                continue;
            }
            datafd = connect_data(cli);
            if (datafd < 0) {
                close(fd);
                send_reply(connfd, cli, 550, "Failed to access.");
                continue;
            }
            make_mode_string(cli, mode, sizeof(mode));
            send_reply(connfd, cli, 150, "Opening %s mode data connection for %s.", mode, arg);
            total = 0;
            while ((n = read(fd, buf, sizeof(buf))) > 0) {
                write(datafd, buf, n);
                total += n;
            }
            close(fd);
            close(datafd);
            write_log(cli, "RETR type=%s bytes=%d", mode, total);
            send_reply(connfd, cli, 226, "Complete transmission.");
            cli->data_port = -1;
        }
        else if (!strcmp(cmd, "STOR")) {
            char *base;
            const char *save_name;

            base = strrchr(arg, '/');
            if (base == NULL) save_name = arg;
            else save_name = base + 1;
            if (cli->data_port < 0) {
                send_reply(connfd, cli, 550, "Failed to access.");
                continue;
            }
            datafd = connect_data(cli);
            if (datafd < 0) {
                send_reply(connfd, cli, 550, "Failed to access.");
                continue;
            }
            fd = open(save_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                close(datafd);
                send_reply(connfd, cli, 550, "Failed transmission.");
                continue;
            }
            make_mode_string(cli, mode, sizeof(mode));
            send_reply(connfd, cli, 150, "Opening %s mode data connection for %s.", mode, arg);
            total = 0;
            while ((n = read(datafd, buf, sizeof(buf))) > 0) {
                write(fd, buf, n);
                total += n;
            }
            close(fd);
            close(datafd);
            write_log(cli, "STOR type=%s bytes=%d", mode, total);
            send_reply(connfd, cli, 226, "Complete transmission.");
            cli->data_port = -1;
        }
        else if (!strcmp(cmd, "QUIT")) {
            send_reply(connfd, cli, 221, "Goodbye");
            write_log(cli, "Client disconnected");
            return;
        }
        else {
            send_reply(connfd, cli, 550, "Invalid command.");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// parse_port_argument
// =============================================================== //
// Input : arg -> PORT argument, cli -> client info
// Output: int - 0 success, -1 fail
// Purpose: Parsing active-mode data address.
///////////////////////////////////////////////////////////////////////////////
static int parse_port_argument(const char *arg, ClientInfo *cli)
{
    int h1, h2, h3, h4, p1, p2;
    int port;

    if (sscanf(arg, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) == 6) {
        if (h1 < 0 || h1 > 255 || h2 < 0 || h2 > 255 ||
            h3 < 0 || h3 > 255 || h4 < 0 || h4 > 255 ||
            p1 < 0 || p1 > 255 || p2 < 0 || p2 > 255) return -1;

        port = p1 * 256 + p2;
        snprintf(cli->data_ip, sizeof(cli->data_ip), "%d.%d.%d.%d", h1, h2, h3, h4);
        cli->data_port = port;
        return 0;
    }

    port = atoi(arg);
    if (port <= 0) return -1;
    cli->data_port = port;
    strncpy(cli->data_ip, cli->ip, sizeof(cli->data_ip) - 1);
    cli->data_ip[sizeof(cli->data_ip) - 1] = '\0';
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// connect_data
// =============================================================== //
// Input : cli -> client data address info
// Output: int - data socket fd or -1
// Purpose: Connecting to client data socket.
///////////////////////////////////////////////////////////////////////////////
static int connect_data(ClientInfo *cli)
{
    int datafd;
    struct sockaddr_in addr;

    datafd = socket(AF_INET, SOCK_STREAM, 0);
    if (datafd < 0) return -1;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(cli->data_ip);
    addr.sin_port = htons(cli->data_port);
    if (connect(datafd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(datafd);
        return -1;
    }
    return datafd;
}

///////////////////////////////////////////////////////////////////////////////
// list_to_fd
// =============================================================== //
// Input : fd -> data socket, arg -> path, list_all/long_format -> list options
// Output: int - sent byte count or -1
// Purpose: Writing directory list to data socket.
///////////////////////////////////////////////////////////////////////////////
static int list_to_fd(int fd, const char *arg, int list_all, int long_format)
{
    struct stat st;
    DIR *dirp;
    struct dirent *dp;
    char full[PATH_BUF];
    int total = 0, n;

    if (stat(arg, &st) < 0) return -1;
    if (!S_ISDIR(st.st_mode)) return list_one(fd, arg, arg, long_format);

    dirp = opendir(arg);
    if (!dirp) return -1;
    while ((dp = readdir(dirp)) != NULL) {
        if (!list_all && dp->d_name[0] == '.') continue;
        snprintf(full, sizeof(full), "%s/%s", arg, dp->d_name);
        n = list_one(fd, full, dp->d_name, long_format);
        if (n > 0) total += n;
    }
    closedir(dirp);
    return total;
}

///////////////////////////////////////////////////////////////////////////////
// list_one
// =============================================================== //
// Input : fd -> data socket, path/name -> file info, long_format -> option
// Output: int - written byte count or -1
// Purpose: Writing one file entry.
///////////////////////////////////////////////////////////////////////////////
static int list_one(int fd, const char *path, const char *name, int long_format)
{
    struct stat st;
    char out[MAX_BUF], mode[11], tbuf[64];
    struct passwd *pw;
    int n;

    if (lstat(path, &st) < 0) return -1;
    if (!long_format) {
        n = snprintf(out, sizeof(out), "%s\n", name);
    } else {
        mode_to_string(st.st_mode, mode);
        time_to_string(st.st_mtime, tbuf, sizeof(tbuf));
        pw = getpwuid(st.st_uid);
        n = snprintf(out, sizeof(out), "%s %3lu %-8s %-8s %8ld %s %s\n",
                     mode, (unsigned long)st.st_nlink,
                     pw ? pw->pw_name : "user", pw ? pw->pw_name : "group",
                     (long)st.st_size, tbuf, name);
    }
    write(fd, out, n);
    return n;
}

///////////////////////////////////////////////////////////////////////////////
// mode_to_string
// =============================================================== //
// Input : mode -> file mode, out -> mode string
// Output: void
// Purpose: Converting file permission to string.
///////////////////////////////////////////////////////////////////////////////
static void mode_to_string(mode_t mode, char *out)
{
    if (S_ISDIR(mode)) out[0] = 'd';
    else if (S_ISLNK(mode)) out[0] = 'l';
    else out[0] = '-';

    if (mode & S_IRUSR) out[1] = 'r'; else out[1] = '-';
    if (mode & S_IWUSR) out[2] = 'w'; else out[2] = '-';
    if (mode & S_IXUSR) out[3] = 'x'; else out[3] = '-';
    if (mode & S_IRGRP) out[4] = 'r'; else out[4] = '-';
    if (mode & S_IWGRP) out[5] = 'w'; else out[5] = '-';
    if (mode & S_IXGRP) out[6] = 'x'; else out[6] = '-';
    if (mode & S_IROTH) out[7] = 'r'; else out[7] = '-';
    if (mode & S_IWOTH) out[8] = 'w'; else out[8] = '-';
    if (mode & S_IXOTH) out[9] = 'x'; else out[9] = '-';

    out[10] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
// time_to_string
// =============================================================== //
// Input : t -> file time, out -> time string
// Output: void
// Purpose: Formatting file time for listing.
///////////////////////////////////////////////////////////////////////////////
static void time_to_string(time_t t, char *out, int size)
{
    strftime(out, size, "%b %d %H:%M", localtime(&t));
}

///////////////////////////////////////////////////////////////////////////////
// check_ip
// =============================================================== //
// Input : client_ip -> client IP address
// Output: int - 1 allowed, 0 denied
// Purpose: Checking access.txt permission.
///////////////////////////////////////////////////////////////////////////////
static int check_ip(const char *client_ip)
{
    FILE *fp = fopen("access.txt", "r");
    char ip[128];
    if (!fp) return 0;
    while (fgets(ip, sizeof(ip), fp)) {
        trim_newline(ip);
        if (!strcmp(ip, client_ip) || !strcmp(ip, "*.*.*.*")) { fclose(fp); return 1; }
    }
    fclose(fp);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// open_passwd_file
// =============================================================== //
// Input : None
// Output: FILE* - opened passwd file or NULL
// Purpose: Opening password file.
///////////////////////////////////////////////////////////////////////////////
static FILE *open_passwd_file(void)
{
    FILE *fp;

    fp = fopen("passwd", "r");
    if (fp == NULL) {
        fp = fopen("passwd.txt", "r");
    }

    return fp;
}

///////////////////////////////////////////////////////////////////////////////
// user_exists
// =============================================================== //
// Input : user -> username
// Output: int - 1 exists, 0 not exists
// Purpose: Checking username in password file.
///////////////////////////////////////////////////////////////////////////////
static int user_exists(const char *user)
{
    FILE *fp = open_passwd_file();
    char line[256], fu[128], fpw[128];
    if (!fp) return 0;
    while (fgets(line, sizeof(line), fp)) {
        fu[0] = fpw[0] = 0;
        sscanf(line, "%127[^:]:%127[^:]", fu, fpw);
        if (!strcmp(user, fu)) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// user_match
// =============================================================== //
// Input : user -> username, passwd -> password
// Output: int - 1 match, 0 not match
// Purpose: Checking username and password.
///////////////////////////////////////////////////////////////////////////////
static int user_match(const char *user, const char *passwd)
{
    FILE *fp = open_passwd_file();
    char line[256], fu[128], fpw[128];
    if (!fp) return 0;
    while (fgets(line, sizeof(line), fp)) {
        fu[0] = fpw[0] = 0;
        sscanf(line, "%127[^:]:%127[^:]", fu, fpw);
        trim_newline(fpw);
        if (!strcmp(user, fu) && !strcmp(passwd, fpw)) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// reply_motd
// =============================================================== //
// Input : connfd -> client socket, cli -> client info
// Output: void
// Purpose: Sending server welcome message.
///////////////////////////////////////////////////////////////////////////////
static void reply_motd(int connfd, ClientInfo *cli)
{
    FILE *fp = fopen("motd", "r");
    char format[MAX_BUF];
    char msg[MAX_BUF];
    char tbuf[128];
    time_t now;

    if (fp) {
        if (!fgets(format, sizeof(format), fp)) {
            snprintf(format, sizeof(format),
                     "sswlab.kw.ac.kr FTP server (version myftp [1.0] %%s)");
        }
        fclose(fp);
        trim_newline(format);
    } else {
        snprintf(format, sizeof(format),
                 "sswlab.kw.ac.kr FTP server (version myftp [1.0] %%s)");
    }

    now = time(NULL);
    strftime(tbuf, sizeof(tbuf), "%a %b %d %H:%M:%S KST %Y", localtime(&now));

    {
        char *pos = strstr(format, "%s");
        if (pos) {
            size_t prefix_len = (size_t)(pos - format);
            snprintf(msg, sizeof(msg), "%.*s%s%s",
                     (int)prefix_len, format, tbuf, pos + 2);
        } else {
            snprintf(msg, sizeof(msg), "%s", format);
        }
    }

    send_reply(connfd, cli, 220, "%s ready.", msg);
}

///////////////////////////////////////////////////////////////////////////////
// send_reply
// =============================================================== //
// Input : fd -> client socket, cli -> client info, code/fmt -> reply text
// Output: int - written byte count
// Purpose: Sending FTP reply and logging it.
///////////////////////////////////////////////////////////////////////////////
static int send_reply(int fd, ClientInfo *cli, int code, const char *fmt, ...)
{
    char msg[MAX_BUF], out[MAX_BUF + 32];
    va_list ap;
    int n;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    if (code > 0) snprintf(out, sizeof(out), "%d %s", code, msg);
    else snprintf(out, sizeof(out), "%s", msg);
    n = write(fd, out, strlen(out));
    write(fd, "\n", 1);
    printf("%s\n", out);
    fflush(stdout);
    if (cli && code > 0) write_log(cli, "Server reply: %s", out);
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
// parse_code_line
// =============================================================== //
// Input : line -> command line, cmd/arg -> parsed result
// Output: int - 0 success
// Purpose: Parsing FTP command and argument.
///////////////////////////////////////////////////////////////////////////////
static int parse_code_line(const char *line, char *cmd, char *arg)
{
    int i = 0;
    cmd[0] = arg[0] = 0;
    sscanf(line, "%63s", cmd);
    while (line[i] && !isspace((unsigned char)line[i])) i++;
    while (line[i] && isspace((unsigned char)line[i])) i++;
    strncpy(arg, line + i, MAX_CMD - 1);
    arg[MAX_CMD - 1] = 0;
    uppercase(cmd);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// split_rename_arg
// =============================================================== //
// Input : arg -> rename argument, oldn/newn -> file names
// Output: int - 0 success, -1 fail
// Purpose: Splitting RNFR/RNTO argument.
///////////////////////////////////////////////////////////////////////////////
static int split_rename_arg(const char *arg, char *oldn, char *newn)
{
    char rnto[64];
    oldn[0] = newn[0] = rnto[0] = 0;
    if (sscanf(arg, "%1023s %63s %1023s", oldn, rnto, newn) != 3) return -1;
    uppercase(rnto);
    if (strcmp(rnto, "RNTO")) return -1;
    return 0;
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
    s[strcspn(s, "\r\n")] = 0;
}

///////////////////////////////////////////////////////////////////////////////
// uppercase
// =============================================================== //
// Input : s -> target string
// Output: void
// Purpose: Changing string to uppercase.
///////////////////////////////////////////////////////////////////////////////
static void uppercase(char *s)
{
    while (*s) { *s = (char)toupper((unsigned char)*s); s++; }
}

///////////////////////////////////////////////////////////////////////////////
// make_mode_string
// =============================================================== //
// Input : cli -> client type, out -> mode string
// Output: void
// Purpose: Making transfer mode string.
///////////////////////////////////////////////////////////////////////////////
static void make_mode_string(ClientInfo *cli, char *out, int size)
{
    if (cli->type == 1) {
        snprintf(out, size, "ascii");
    }
    else {
        snprintf(out, size, "binary");
    }
}

///////////////////////////////////////////////////////////////////////////////
// log_open
// =============================================================== //
// Input : None
// Output: void
// Purpose: Opening server logfile.
///////////////////////////////////////////////////////////////////////////////
static void log_open(void)
{
    g_logfp = fopen("logfile", "a");
}

///////////////////////////////////////////////////////////////////////////////
// write_log
// =============================================================== //
// Input : cli -> client info, fmt -> log format
// Output: void
// Purpose: Writing server log message.
///////////////////////////////////////////////////////////////////////////////
static void write_log(ClientInfo *cli, const char *fmt, ...)
{
    char timebuf[64], msg[MAX_BUF];
    time_t now = time(NULL);
    va_list ap;
    FILE *fp;

    if (g_logfp != NULL) fp = g_logfp;
    else fp = fopen("logfile", "a");

    if (!fp) return;

    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    if (cli) {
        fprintf(fp, "[%s] [IP:%s] [Port:%d] [User:%s] %s\n", timebuf, cli->ip, cli->port, cli->user, msg);
    }
    else {
        fprintf(fp, "[%s] %s\n", timebuf, msg);
    }
    fflush(fp);
    if (!g_logfp) fclose(fp);
}

///////////////////////////////////////////////////////////////////////////////
// sigint_handler
// =============================================================== //
// Input : signo -> signal number
// Output: void
// Purpose: Handling server termination signal.
///////////////////////////////////////////////////////////////////////////////
static void sigint_handler(int signo)
{
    (void)signo;
    write_log(NULL, "Server is terminated");
    if (g_listenfd >= 0) close(g_listenfd);
    if (g_logfp) fclose(g_logfp);
    _exit(0);
}

///////////////////////////////////////////////////////////////////////////////
// sigchld_handler
// =============================================================== //
// Input : signo -> signal number
// Output: void
// Purpose: Removing terminated child processes.
///////////////////////////////////////////////////////////////////////////////
static void sigchld_handler(int signo)
{
    (void)signo;
    while (waitpid(-1, NULL, WNOHANG) > 0) ;
}
