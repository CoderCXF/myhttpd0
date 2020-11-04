#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <ctype.h>
#include <string.h>

#define default_port 9876
#define OPENMAX 2048

int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return(i);
}

int initSocket(u_short port) {
    int opt = 1;
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("sock error");
        exit(-1);
    }
    /*Reuse port*/
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind error");
        exit(-1);
    }
    if ((listen(listenfd, 128)) == -1) {
        perror("bind error");
        exit(-1);
    }
    printf("Waiting for connect...\n");
    return listenfd;
}

void deal_accept(int lfd, int epfd) {
    int cfd = 0;
    int ret = 0;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    cfd = accept(lfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (cfd == -1) {
        perror("accept error");
        exit(-1);
    }
    /*noblock setting for cfd*/
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);

    struct epoll_event evt;
    /*ET modle*/
    evt.events = EPOLLIN | EPOLLET;
    evt.data.fd = cfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &evt);
    if (ret == -1) {
        perror("epoll_ctl add cfd error:");
        exit(-1);
    }
    return ;
}

/*read http request data from browser*/
void accept_request(int sockfd, int epfd) {
    // int n = 0;
    // int j = 0;
    // char buf[BUFSIZ];
    // struct epoll_event evt;

    // n = read(sockfd, buf, sizeof(buf));
    // if (n == 0) {
    //     close(sockfd);
    //     epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
    //     printf("client %d close connection\n", sockfd);
    //     } else if (n < 0) {
    //         if (errno != EINTR) {
    //             perror("read error");
    //             exit(-1);
    //         } 
    //     } else {
    //         // printf("read from client!\n");
    //         for (j = 0; j < n; j++) {
    //             buf[j] = toupper(buf[j]);
    //         }
    //         write(sockfd, buf, n);
    //     }
    char buf[BUFSIZ];
    int numbers = 0;
    numbers = get_line(sockfd, buf, sizeof(buf));
    if () {
        
    }

    return ;
}

void epoll_listen(int lfd) {
    int nready;
    int i;
    int sockfd;
    struct epoll_event evt, evts[OPENMAX];
    int epfd = epoll_create(OPENMAX);
    if (epfd == -1) {
        perror("epoll_create error");
        exit(-1);
    }
    evt.events = EPOLLIN;
    evt.data.fd = lfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &evt);
    while (1) {
        nready = epoll_wait(epfd, evts, OPENMAX, -1);
        if (nready == -1) {
            perror("epoll_wait error:");
            exit(-1);
        }
        for (i = 0; i < nready; ++i) {
            if (!(evts[i].events & EPOLLIN)) {
                continue;
            }
            sockfd = evts[i].data.fd;
            if (sockfd == lfd) {
                deal_accept(lfd, epfd);
            } else {
                accept_request(sockfd, epfd);
            }
        }
    }
    close(lfd);
    close(epfd);
    return;
}

int main (int argc, char **argv) {
    int lfd;
    u_short port;
    int ret = 0;
    if (argc < 3) {
        printf("./server port path\n");
        exit(-1);
    }
    port = atoi(argv[1]);
    // ret = chdir(argv[2]);
    if (ret == -1) {
        perror("chdir error");
        exit(-1);
    }
    lfd = initSocket(port);
    epoll_listen(lfd);
    close(lfd);
    return 0;
}