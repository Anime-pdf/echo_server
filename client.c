#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <arpa/inet.h>

void *get_addr(struct sockaddr* sa) {
    if(sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)&sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)&sa)->sin6_addr);
}

int main(int argc, char *argv[]) {

    if(argc != 3) {
        fprintf(stderr, "Usage: Client <ip/hostname> <port>\n");
        exit(1);
    }

    struct addrinfo hints, *result, *p;

    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status;
    if((status = getaddrinfo(argv[1], argv[2], &hints, &result)) != 0) {
        fprintf(stderr, "client: gai: %s", gai_strerror(status));
        exit(2);
    }

    for (p = result; p != NULL; p = p->ai_next) {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    char ip[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_addr(p->ai_addr), ip, INET6_ADDRSTRLEN);
    printf("client: connection to %s:%s\n", ip, argv[2]);

    freeaddrinfo(result);

    char recv_buf[1024];
    char send_buf[1024];

    while(recv(sockfd, recv_buf, sizeof recv_buf, 0) > 0) {
        printf("Got: %s\n", recv_buf);
        scanf(" %1023[^\n]", send_buf);
        if(send(sockfd, send_buf, sizeof send_buf, 0) == -1) {
            perror("client: send");
            close(sockfd);
            exit(3);
        }
    }

    close(sockfd);
    return 0;
}