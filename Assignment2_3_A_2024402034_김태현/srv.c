#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_BUFF 1024
#define SEND_BUFF 8192
#define MAX_BUF 1024
#define MAX_CLIENT 100

typedef struct
{
    pid_t pid;
    int port;
    time_t start_time;
} ClientInfo;

int client_info(struct sockaddr_in *cliaddr);
int cmd_process(char *buff, char *result_buff);

void append_num(char *result_buff, long n);
void writePermissions(mode_t mode, char *result_buff);
int myls(char *option, char *path, char *result_buff);

void sigchldHandler(int signum);
void sigalrmHandler(int signum);
void sigintHandler(int signum);

void add_client(pid_t pid, int port);
void remove_client(pid_t pid);
void print_client_list(void);

ClientInfo client_list[MAX_CLIENT];
int client_count = 0;
int server_fd_global = -1;

int main(int argc, char **argv)
{
    char buff[MAX_BUFF];
    char result_buff[SEND_BUFF];
    int n;

    if (argc != 2)
    {
        write(STDOUT_FILENO, "입력 규격: ./srv [PORT]\n",
              strlen("입력 규격: ./srv [PORT]\n"));
        exit(1);
    }

    int serverfd;
    int connfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;
    socklen_t clilen;

    signal(SIGCHLD, sigchldHandler);
    signal(SIGALRM, sigalrmHandler);
    signal(SIGINT, sigintHandler);

    serverfd = socket(PF_INET, SOCK_STREAM, 0);
    if (serverfd < 0)
    {
        write(STDERR_FILENO, "socket() error!!\n",
              strlen("socket() error!!\n"));
        exit(1);
    }

    server_fd_global = serverfd;

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

    alarm(10);

    while (1)
    {
        pid_t pid;

        clilen = sizeof(cliaddr);
        connfd = accept(serverfd, (struct sockaddr *)&cliaddr, &clilen);

        if (connfd < 0)
        {
            if (errno == EINTR)
                continue;

            write(STDERR_FILENO, "accept() error!!\n",
                  strlen("accept() error!!\n"));
            continue;
        }

        pid = fork();

        if (pid < 0)
        {
            close(connfd);
            continue;
        }

        if (pid == 0)
        {
            close(serverfd);

            while (1)
            {
                memset(buff, 0, MAX_BUFF);
                memset(result_buff, 0, SEND_BUFF);

                n = read(connfd, buff, MAX_BUFF - 1);

                if (n <= 0)
                    break;

                buff[n] = '\0';

                cmd_process(buff, result_buff);

                write(connfd, result_buff, strlen(result_buff));

                if (strcmp(result_buff, "QUIT") == 0)
                {
                    close(connfd);
                    exit(0);
                }
            }

            close(connfd);
            exit(0);
        }
        else
        {
            client_info(&cliaddr);
            printf("Child Process ID : %d\n", pid);

            add_client(pid, ntohs(cliaddr.sin_port));

            print_client_list();

            alarm(10);

            close(connfd);
        }
    }

    close(serverfd);
    return 0;
}

int client_info(struct sockaddr_in *cliaddr)
{
    char port_buff[100];

    write(STDOUT_FILENO, "==========Client info==========\n",
          strlen("==========Client info==========\n"));

    write(STDOUT_FILENO, "client IP: ", strlen("client IP: "));
    write(STDOUT_FILENO, inet_ntoa(cliaddr->sin_addr),
          strlen(inet_ntoa(cliaddr->sin_addr)));
    write(STDOUT_FILENO, "\n", 1);

    sprintf(port_buff, "client port: %d\n", ntohs(cliaddr->sin_port));
    write(STDOUT_FILENO, port_buff, strlen(port_buff));

    write(STDOUT_FILENO, "===============================\n",
          strlen("===============================\n"));

    return 0;
}

int cmd_process(char *buff, char *result_buff)
{
    char FTPcommand[MAX_BUFF];

    memset(FTPcommand, 0, MAX_BUFF);
    strcpy(FTPcommand, buff);
    FTPcommand[strcspn(FTPcommand, "\n")] = '\0';

    if (strncmp(FTPcommand, "NLST", 4) == 0)
    {
        char *option = NULL;
        char *path = ".";

        char *token = strtok(FTPcommand, " ");
        while ((token = strtok(NULL, " ")) != NULL)
        {
            if (token[0] == '-')
                option = token;
            else
                path = token;
        }

        printf("ls");
        if (option != NULL)
            printf(" %s", option);
        if (path != NULL && strcmp(path, ".") != 0)
            printf(" %s", path);
        printf(" [%d]\n", getpid());

        myls(option, path, result_buff);
    }

    else if (strncmp(FTPcommand, "LIST", 4) == 0)
    {
        char *option = "-al";
        char *path = ".";

        strtok(FTPcommand, " ");
        char *token = strtok(NULL, " ");

        if (token != NULL)
            path = token;

        printf("dir");
        if (path != NULL && strcmp(path, ".") != 0)
            printf(" %s", path);
        printf(" [%d]\n", getpid());

        myls(option, path, result_buff);
    }

    else if (strncmp(FTPcommand, "PWD", 3) == 0)
    {
        char currentWD[MAX_BUF];

        printf("pwd [%d]\n", getpid());

        if (getcwd(currentWD, sizeof(currentWD)) == NULL)
        {
            strcpy(result_buff, "Error: directory not found\n");
            return 0;
        }

        strcat(result_buff, "'");
        strcat(result_buff, currentWD);
        strcat(result_buff, "' is current directory\n");
    }

    else if (strncmp(FTPcommand, "CWD", 3) == 0)
    {
        strtok(FTPcommand, " ");
        char *targetWD = strtok(NULL, " ");

        printf("cd");
        if (targetWD != NULL)
            printf(" %s", targetWD);
        printf(" [%d]\n", getpid());

        if (targetWD == NULL || chdir(targetWD) == -1)
        {
            strcpy(result_buff, "Error: directory not found\n");
            return 0;
        }

        char absolutePath[MAX_BUF];
        getcwd(absolutePath, sizeof(absolutePath));

        strcat(result_buff, "CWD ");
        strcat(result_buff, targetWD);
        strcat(result_buff, "\n");

        strcat(result_buff, "'");
        strcat(result_buff, absolutePath);
        strcat(result_buff, "' is current directory\n");
    }

    else if (strncmp(FTPcommand, "CDUP", 4) == 0)
    {
        printf("cd .. [%d]\n", getpid());

        if (chdir("..") == -1)
        {
            strcpy(result_buff, "Error: directory not found\n");
            return 0;
        }

        char currentWD[MAX_BUF];
        getcwd(currentWD, sizeof(currentWD));

        strcat(result_buff, "CDUP\n");
        strcat(result_buff, "'");
        strcat(result_buff, currentWD);
        strcat(result_buff, "' is current directory\n");
    }

    else if (strncmp(FTPcommand, "MKD", 3) == 0)
    {
        strtok(FTPcommand, " ");
        char *dirName;

        printf("mkdir");

        while ((dirName = strtok(NULL, " ")) != NULL)
        {
            printf(" %s", dirName);

            if (mkdir(dirName, 0775) == -1)
            {
                strcat(result_buff, "Error: cannot create directory '");
                strcat(result_buff, dirName);
                strcat(result_buff, "': File exists\n");
                continue;
            }

            strcat(result_buff, "MKD ");
            strcat(result_buff, dirName);
            strcat(result_buff, "\n");
        }

        printf(" [%d]\n", getpid());
    }

    else if (strncmp(FTPcommand, "DELE", 4) == 0)
    {
        strtok(FTPcommand, " ");
        char *fileName;

        printf("delete");

        while ((fileName = strtok(NULL, " ")) != NULL)
        {
            printf(" %s", fileName);

            if (unlink(fileName) == -1)
            {
                strcat(result_buff,
                       "Error: failed to stat the file or directory\n");
                continue;
            }

            strcat(result_buff, "DELE ");
            strcat(result_buff, fileName);
            strcat(result_buff, "\n");
        }

        printf(" [%d]\n", getpid());
    }

    else if (strncmp(FTPcommand, "RMD", 3) == 0)
    {
        strtok(FTPcommand, " ");
        char *dirName;

        printf("rmdir");

        while ((dirName = strtok(NULL, " ")) != NULL)
        {
            printf(" %s", dirName);

            if (rmdir(dirName) == -1)
            {
                strcat(result_buff, "Error: failed to remove '");
                strcat(result_buff, dirName);
                strcat(result_buff, "'\n");
                continue;
            }

            strcat(result_buff, "RMD ");
            strcat(result_buff, dirName);
            strcat(result_buff, "\n");
        }

        printf(" [%d]\n", getpid());
    }

    else if (strncmp(FTPcommand, "RNFR", 4) == 0)
    {
        strtok(FTPcommand, " ");
        char *oldname = strtok(NULL, " ");
        strtok(NULL, " ");
        char *newname = strtok(NULL, " ");

        printf("rename");
        if (oldname != NULL)
            printf(" %s", oldname);
        if (newname != NULL)
            printf(" %s", newname);
        printf(" [%d]\n", getpid());

        if (access(newname, F_OK) == 0)
        {
            strcpy(result_buff, "Error: name to change already exists\n");
            return 0;
        }

        if (rename(oldname, newname) == -1)
        {
            strcpy(result_buff, "Error: failed to rename\n");
            return 0;
        }

        strcat(result_buff, "RNFR ");
        strcat(result_buff, oldname);
        strcat(result_buff, "\nRNTO ");
        strcat(result_buff, newname);
        strcat(result_buff, "\n");
    }

    else if (strncmp(FTPcommand, "RETR", 4) == 0)
    {
        strtok(FTPcommand, " ");
        char *filename = strtok(NULL, " ");

        printf("get");
        if (filename != NULL)
            printf(" %s", filename);
        printf(" [%d]\n", getpid());

        char pathFR[MAX_BUF];
        strcpy(pathFR, "server_root/");
        strcat(pathFR, filename);

        char pathTO[MAX_BUF];
        strcpy(pathTO, "client_root/");
        strcat(pathTO, filename);

        int fdFR = open(pathFR, O_RDONLY);
        if (fdFR == -1)
        {
            strcat(result_buff, "Error: '");
            strcat(result_buff, filename);
            strcat(result_buff, "' does not exist in server_root\n");
            return 0;
        }

        int fdTO = open(pathTO, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        char buf[MAX_BUF];
        ssize_t nread;
        ssize_t total_bytes = 0;

        while ((nread = read(fdFR, buf, MAX_BUF)) > 0)
        {
            write(fdTO, buf, nread);
            total_bytes += nread;
        }

        close(fdFR);
        close(fdTO);

        strcat(result_buff, "RETR ");
        strcat(result_buff, filename);
        strcat(result_buff, "\nOK: ");
        append_num(result_buff, total_bytes);
        strcat(result_buff, " bytes copied to client_root\n");
    }

    else if (strncmp(FTPcommand, "STOR", 4) == 0)
    {
        strtok(FTPcommand, " ");
        char *filename = strtok(NULL, " ");

        printf("put");
        if (filename != NULL)
            printf(" %s", filename);
        printf(" [%d]\n", getpid());

        char pathFR[MAX_BUF];
        strcpy(pathFR, "client_root/");
        strcat(pathFR, filename);

        char pathTO[MAX_BUF];
        strcpy(pathTO, "server_root/");
        strcat(pathTO, filename);

        int fdFR = open(pathFR, O_RDONLY);
        int fdTO = open(pathTO, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        char buf[MAX_BUF];
        ssize_t nread;
        ssize_t total_bytes = 0;

        while ((nread = read(fdFR, buf, MAX_BUF)) > 0)
        {
            write(fdTO, buf, nread);
            total_bytes += nread;
        }

        close(fdFR);
        close(fdTO);

        strcat(result_buff, "STOR ");
        strcat(result_buff, filename);
        strcat(result_buff, "\nOK: ");
        append_num(result_buff, total_bytes);
        strcat(result_buff, " bytes copied to server_root (expected ");
        append_num(result_buff, total_bytes);
        strcat(result_buff, " bytes)\n");
    }

    else if (strncmp(FTPcommand, "QUIT", 4) == 0)
    {
        printf("quit [%d]\n", getpid());
        strcpy(result_buff, "QUIT");
    }

    else
    {
        strcpy(result_buff, "Error: invalid command\n");
    }

    return 0;
}
void append_num(char *result_buff, long n)
{
    char buf[32];
    sprintf(buf, "%ld", n);
    strcat(result_buff, buf);
}

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
    int print_count = 0;

    if (!S_ISDIR(st.st_mode))
    {
        if (long_fmt)
        {
            writePermissions(st.st_mode, result_buff);

            append_num(result_buff, st.st_nlink);
            strcat(result_buff, " ");

            struct passwd *pw = getpwuid(st.st_uid);
            if (pw != NULL)
                strcat(result_buff, pw->pw_name);

            strcat(result_buff, " ");

            struct group *gr = getgrgid(st.st_gid);
            if (gr != NULL)
                strcat(result_buff, gr->gr_name);

            strcat(result_buff, " ");

            append_num(result_buff, st.st_size);
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

            append_num(result_buff, fst.st_nlink);
            strcat(result_buff, " ");

            struct passwd *pw = getpwuid(fst.st_uid);
            if (pw != NULL)
                strcat(result_buff, pw->pw_name);

            strcat(result_buff, " ");

            struct group *gr = getgrgid(fst.st_gid);
            if (gr != NULL)
                strcat(result_buff, gr->gr_name);

            strcat(result_buff, " ");

            append_num(result_buff, fst.st_size);
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
            strcat(result_buff, namelist[i]->d_name);

            if (S_ISDIR(fst.st_mode))
                strcat(result_buff, "/");

            print_count++;

            if (print_count % 5 == 0)
                strcat(result_buff, "\n");
            else
                strcat(result_buff, "\t");
        }

        free(namelist[i]);
    }

    if (!long_fmt && print_count % 5 != 0)
        strcat(result_buff, "\n");

    free(namelist);

    return 0;
}

void add_client(pid_t pid, int port)
{
    if (client_count >= MAX_CLIENT)
        return;

    client_list[client_count].pid = pid;
    client_list[client_count].port = port;
    client_list[client_count].start_time = time(NULL);
    client_count++;
}

void remove_client(pid_t pid)
{
    for (int i = 0; i < client_count; i++)
    {
        if (client_list[i].pid == pid)
        {
            printf("Client(%d)'s Release\n", pid);

            for (int j = i; j < client_count - 1; j++)
                client_list[j] = client_list[j + 1];

            client_count--;
            return;
        }
    }
}

void print_client_list(void)
{
    time_t now = time(NULL);

    printf("Current Number of Client : %d\n", client_count);
    printf("PID\tPORT\tTIME\n");

    for (int i = 0; i < client_count; i++)
    {
        printf("%d\t%d\t%ld\n",
               client_list[i].pid,
               client_list[i].port,
               now - client_list[i].start_time);
    }
}

void sigchldHandler(int signum)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        remove_client(pid);
    }
}

void sigalrmHandler(int signum)
{
    print_client_list();
    alarm(10);
}

void sigintHandler(int signum)
{
    printf("\nServer will be terminated.\n");

    for (int i = 0; i < client_count; i++)
    {
        kill(client_list[i].pid, SIGTERM);
    }

    if (server_fd_global >= 0)
        close(server_fd_global);

    exit(0);
}