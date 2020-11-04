#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <string.h>
#define SERV_IP "127.0.0.1"
#define SERV_PORT 9876
int main() {
    int cfd;
    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1) {
        perror("sock error");
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, SERV_IP, &serv_addr.sin_addr.s_addr);
    if (connect(cfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("conncect error");
        exit(1);
    };

    //write info
    char buf[BUFSIZ];
    int n;
    while (1) {
        fgets(buf, sizeof(buf), stdin);
        write(cfd, buf, strlen(buf));
        n = read(cfd, buf, sizeof(buf));
        write(STDOUT_FILENO, buf, n);
    }
    close(cfd);
    return 0;
}