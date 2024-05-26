#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

void sigchld_handler(int s)
{
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

void *get_addr(struct sockaddr* sa) {
    if(sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)&sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)&sa)->sin6_addr);
}

int main() {
    const char *port = "8303";
    char welcome[256] = "Welcome to the echo server, escape character is 'q'";
    char goodbye[256] = "Thanks for using this and goodbye!";

    int sockfd, newfd;

    struct addrinfo hints;
    struct addrinfo *result, *p;
    struct sockaddr_storage client;

    memset(&hints, 0, sizeof  hints);
    hints.ai_socktype = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status;
    if((status = getaddrinfo(NULL, port, &hints, &result))) {
        fprintf(stderr, "server: gai: %s", gai_strerror(status));
        exit(1);
    }

    int yes=1;
    for (p = result; p != NULL; p = p->ai_next) {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
            perror("server: setsockopt");
            exit(2);
        }

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("server: bind");
            continue;
        }

        break;
    }

    if(p == NULL) {
        fprintf(stderr, "server: bind failed");
        exit(3);
    }

    freeaddrinfo(result);

    if(listen(sockfd, 10) == -1) {
        perror("server: listen");
        exit(4);
    }

    struct sigaction sa;
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(5);
    }

    while (1) {
        socklen_t c_size = sizeof client;
        if((newfd = accept(sockfd,(struct sockaddr*)&client, &c_size) == -1)) {
            perror("server: accept");
            continue;
        }

        char ip[INET6_ADDRSTRLEN];
        inet_ntop(client.ss_family, get_addr((struct sockaddr*)&client), ip, INET6_ADDRSTRLEN);
        printf("server: got connection from %s", ip);

        if(!fork()) {
            close(sockfd);
            if(send(newfd, welcome, sizeof welcome, 0) == -1) {
                perror("server: send");
                close(newfd);
                exit(6);
            }

            char recv_buf[1024];
            while(strcmp(recv_buf, "q") != 0) {
                if(recv(newfd, recv_buf, sizeof recv_buf, 0) == -1) {
                    perror("server: recv");
                    continue;
                }

                if(send(newfd, recv_buf, sizeof recv_buf, 0) == -1) {
                    perror("server: send");
                    close(newfd);
                    exit(7);
                }
            }

            if(send(newfd, goodbye, sizeof goodbye, 0) == -1) {
                perror("server: send");
                close(newfd);
                exit(8);
            }

            close(newfd);
            exit(0);
        }

        close(newfd);
    }

    return 0;
}