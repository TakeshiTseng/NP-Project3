#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include "passivesock.h"
#include <fcntl.h>


void handleSIGCHLD() {
    int stat;
    /*Kills all the zombie processes*/
    while(waitpid(-1, &stat, WNOHANG) > 0);
    // while(wait3(&stat, WNOHANG, (struct rusage*)0)>=0);
}

int read_line(int fd, char* buffer);
void clear_buf(int sock_fd) {

    char buf[128];
    while(read_line(sock_fd, buf) > 0) {
        /* for debug */
        int a;
        a = 0;
    }
}

void print404(int sock_fd) {
    close(0);
    close(1);
    close(2);
    dup(sock_fd);
    dup(sock_fd);
    dup(sock_fd);
    printf("HTTP/1.1 404 Not Found\r\n");
    fflush(stdout);
    printf("Content-type: text/html\r\n\r\n");
    fflush(stdout);
    printf("<html>\n<body>");
    fflush(stdout);
    printf("<img src=\"http://zenit.senecac.on.ca/wiki/imgs/404-not-found.gif\"></img>\n");
    fflush(stdout);
    printf("</body>\n</html>\n");
    fflush(stdout);
}

void execute_cgi(char* url, int sock_fd) {

    printf("Processing url: %s\n", url);
    char** res;
    char** res2;
    int num_tok;
    str_split(url, "/", &res, &num_tok);
    if(num_tok < 2) {
        print404(sock_fd);
        return;
    }
    char path[128];
    bzero(path, 128);
    strcpy(path, res[0]);
    chdir("/u/gcs/103/0356063/public_html");
    sprintf(path, "/u/gcs/103/0356063/public_html");
    // strcpy(path, res[0]);
    printf("Path: %s\n", path);
    setenv("PATH", path, 1);

    char cgi_name[128];
    char html_name[128];
    char qstring[256];
    char filepath[512];
    bzero(filepath, 512);

    if(is_match(url, ".*\\?.*") == 1 || is_match(url, ".*cgi") == 1) {
        // might be cgi XD
        str_split(res[1], "?", &res2, &num_tok);
        bzero(cgi_name, 128);
        strcpy(cgi_name, res2[0]);
        printf("Cgi name: %s\n", cgi_name);
        sprintf(filepath, "%s/%s", path, cgi_name);
        bzero(qstring, 256);
        strcpy(qstring, res2[1]);
        printf("Query string: %s\n", qstring);
        setenv("QUERY_STRING", qstring, 1);
    } else {
        // just html =_=
        bzero(html_name, 128);
        strcpy(html_name, res[1]);
        sprintf(filepath, "%s/%s", path, html_name);
    }

    printf("Filepath: %s\n", filepath);
    struct stat sb;

    close(0);
    close(1);
    close(2);
    dup(sock_fd);
    dup(sock_fd);
    dup(sock_fd);

    if(stat(filepath, &sb) == -1) {
        // 404 not found!
        print404(sock_fd);
        return;
    }

    if(strlen(cgi_name) != 0 && is_match(cgi_name, "*.cgi")) {
        char* args[] = {cgi_name, NULL};
        printf("HTTP/1.1 404 Not Found\r\n");
        fflush(stdout);
        int p = fork();
        if(p == 0) {
            execvp(cgi_name, args);
        } else {
            int status;
            waitpid(p, &status, 0);
        }
    } else {
        int filefd = open(filepath, O_RDWR, 0);
        printf("HTTP/1.1 200 OK\n");
        fflush(stdout);
        printf("Content-type: text/html\r\n\r\n");
        fflush(stdout);
        char buf[1024];
        bzero(buf, 1024);
        int len = read(filefd, buf, 1024);
        while(len > 0) {
            write(sock_fd, buf, len);
            fflush(stdout);
            bzero(buf, 1024);
            len = read(filefd, buf, 1024);
        }
    }

}

int read_line(int fd, char* buffer) {
    int count = 0;
    char buf[1];
    while(read(fd, buf, 1) > 0) {
        if(buf[0] == '\r')continue;
        buffer[count++] = buf[0];
        if(buf[0] == '\n')break;
    }
    return count;
}

int main(int argc, const char *argv[])
{
    signal(SIGCHLD, handleSIGCHLD);
    srand(time(NULL));
    struct sockaddr_in my_addr;
    struct sockaddr_in client_addr;
    int port = 2000 + rand() % 100;
    printf("Port : %d\n", port);
    char port_str[5];
    sprintf(port_str, "%d", port);
    int sc_fd = passivesock(port_str, "tcp", 5);
    printf("accepting.....\n");
    int addrlen = sizeof(client_addr);

    while(1) {
        int new_client_sock = accept(sc_fd, (struct sockaddr *)&client_addr, (socklen_t*)&addrlen);
        printf("Accept !\n");
        usleep(100000); // sleep for 0.1 sec
        int flag = fcntl(new_client_sock, F_GETFL, 0);
        fcntl(new_client_sock, F_SETFL, flag | O_NONBLOCK);
        int pid = fork();
        if(pid == 0) {
            close(sc_fd);
            char buffer[1024];
            char tmpbuf[1024];
            bzero(buffer, 1024);

            // firset line is GET command
            if(read_line(new_client_sock, buffer) > 0) {
                // parser tokens
                char** res;
                int num_tok;
                str_split(buffer, " ", &res, &num_tok);
                if(num_tok != 3) {
                    return -1;
                } else {
                    // copy url
                    char url[1024];
                    strcpy(url, res[1]);
                    while(read_line(new_client_sock, buffer) > 0) {
                        /* clear buffer */
                        printf("%s\n", buffer);
                        usleep(10000); // sleep for 0.01 sec
                    }
                    flag = fcntl(new_client_sock, F_GETFL, 0);
                    flag = flag & (~O_NONBLOCK);
                    fcntl(new_client_sock, F_SETFL, flag);
                    execute_cgi(url, new_client_sock);
                    close(new_client_sock);
                    return 0;
                }

            } else {
                // didn't get GET, break.
                return -1;
            }
            return 0;
        } else {
            close(new_client_sock);
        }
    }
    return 0;
}
