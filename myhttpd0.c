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
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define default_port 9999
#define OPENMAX 2048

/**
 * @description: getline http request message
 * @param: 1. sock fd;2. buffer
 * @return: bytes
 */
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

    return (i);
}

/**
 * @description: obtain file type
 * @param : file name
 * @return : const char *
 */
const char *get_file_type(const char *name)
{
    char* dot;
    dot = strrchr(name, '.');   
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav" ) == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";
    return "text/plain; charset=utf-8";
}
int hexit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return 0;
}
/**
 * @description: encode:chinese--->utf8
 * @param {*}
 * @return {*}
 */
void encode_str(char *to, int tosize, const char *from) {
    int tolen;
    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from)
    {
        if (isalnum(*from) || strchr("/_.-~", *from) != (char *)0) {
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}

/**
 * @description: decode:utf8--->chinese
 * @param :
 * @return : void
 */
void decode_str(char *to, char *from) {
    for( ; *from != '\0'; ++to, ++from) {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
            *to = hexit(from[1]) * 16 + hexit(from[2]);
            from += 2;
        } else {
            *to = *from;
        }
    }
    *to = '\0';
}
/**
 * @description: 404 notfound  web
 * @param : client fd
 * @return : void
 */
void notfound(int cfd)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><head><TITLE>Not Found</TITLE></head>\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><h1 style=\"text-align:center\">404 not found</h1>\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "<p style=\"text-align: center\">myhttpd0</p>\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(cfd, buf, strlen(buf), 0);
    return;
}

/**
 * @description: Only GET and POST request
 * @param : client fd
 * @return : void
 */
void unimplemented(int cfd)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(cfd, buf, strlen(buf), 0);
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(cfd, buf, strlen(buf), 0);
    return;
}

/**
 * @description: 500 Internal Server Error  web
 * @param: client fd
 * @return : void
 */
void internal_server_error(int cfd)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><head><TITLE>500 Error</TITLE></head>\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><h1 style=\"text-align:center\">500 Internal Server Error</h1>\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "<p style=\"text-align: center\">myhttpd0</p>\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(cfd, buf, strlen(buf), 0);
    return;
}
/**
 * @description: send response headers
 * @param: param 3 is flag whether this file is directory
 * @return: void
 */
void send_headers(int cfd, const char *fileName, int flag) {
    char buf[1024] = {0};
    const char *file_type;
    /*juege file type by fileName*/
    if (flag) {
        file_type = "text/html; charset=utf-8";
    } else {
        file_type = get_file_type(fileName);
    }
    printf("file_type: %s\n", file_type);
    strcpy(buf, "HTTP/1.1 200 OK\r\n");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: %s\r\n", file_type);
    send(cfd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(cfd, buf, strlen(buf), 0);
    return ;
}

/**
 * @description: send file to browser
 * @param : 1. client fd; 2. file name
 * @return : void
 */
void send_resource(int cfd, const char *fileName) {
    int fd = 0;
    int n = 0;
    char buf[1024];
    if ((fd = open(fileName, O_RDONLY)) == -1) {
        perror("open file error:");
        /*how to deal*/
    }
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        send(cfd, buf, n, 0);
    }
    close(fd);
    return ;
}

/**
 * @description: send directory info to browser
 * @param : 1.client fd; 2.directory name
 * @return : void
 */
void send_dir_info(int cfd, const char *dirName) {
    int i = 0, num = 0;
    int ret = -1;
    char buf[1024] = {0}, path[1024] = {0};
    char enstr[1024];
    char *li_name;
    strcpy(buf, "<!DOCTYPE html>");
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "<html><head><title>Directory: %s </title></head>", dirName);
    send(cfd, buf, strlen(buf), 0);
    sprintf(buf, "<body><h1>Directory: %s</h1><ol>", dirName);
    send(cfd, buf, strlen(buf), 0);

    struct dirent **name_list;
    num = scandir(dirName, &name_list, NULL, alphasort);
    
    for (i = 0; i < num; ++i) {
        li_name = name_list[i]->d_name;
        sprintf(path, "%s/%s", dirName, li_name);
        struct stat st;
        stat(path, &st);
        encode_str(enstr, sizeof(enstr), li_name);
        if (S_ISREG(st.st_mode)) {
            sprintf(buf, "<li><a href=\"%s\">%s</a></li>", enstr, li_name);
        } else if (S_ISDIR(st.st_mode)) {
            sprintf(buf, "<li><a href=\"%s/\">%s/</a></li>", enstr, li_name);
        }
        ret = send(cfd, buf, strlen(buf), 0);
        if (ret == -1) {
        if (errno == EAGAIN || errno == EINTR) {
            perror("send error");
            continue;
        } else {
            perror("send error");
            exit(-1);
        }
    }
    }
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "</ol></body></html>");
    send(cfd, buf, strlen(buf), 0);
    printf("send message OK !\n");
    return ;
}

/**
 * @description: Handle GET request
 * @param : 1. client fd;2. file name
 * @return : void
 */
void handle_GET(int cfd, const char *fileName)
{
    struct stat file_stat;
    int ret = 0;
    printf("file name:%s\n", fileName);
    /*judge this file wheher exist*/
    if ((ret = stat(fileName, &file_stat)) == -1) {
        notfound(cfd); /*404 notfound*/
        perror("stat error:");
    }
    /*regular file or directory*/
    if (S_ISREG(file_stat.st_mode)) {
        /*reponse message : stat line*/
        send_headers(cfd, fileName, 0);
        /*reponse message : reponse head*/
        send_resource(cfd, fileName);
    }
    /*S_ISDIR*/
    if (S_ISDIR(file_stat.st_mode)) {
        //TODO: paramter 3
        send_headers(cfd, fileName, 1);
        send_dir_info(cfd, fileName);
    }
    return;
}
/**
 * @description: Handle POST request
 * @param : 
 * @return : void
 */
void handle_POST(int cfd, const char *fileName) {

    return ;
}

/**
 * @description: Init sock/bind/listen
 * @param {*}
 * @return {*}
 */
int initSocket(u_short port)
{
    int opt = 1;
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
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
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind error");
        exit(-1);
    }
    if ((listen(listenfd, 128)) == -1)
    {
        perror("bind error");
        exit(-1);
    }
    printf("Waiting for connect on %d...\n", port);
    return listenfd;
}

/**
 * @description: Epoll listen event
 * @param {*}
 * @return {*}
 */
void deal_accept(int lfd, int epfd)
{
    int cfd = 0;
    int ret = 0;
    char IPStr[1024];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    cfd = accept(lfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (cfd == -1)
    {
        perror("accept error");
        exit(-1);
    }
    printf("client ip: %s, client port: %d\n", inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, IPStr, sizeof(IPStr)), ntohs(client_addr.sin_port));
    /*noblock setting for cfd*/
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);

    struct epoll_event evt;
    /*ET modle*/
    evt.events = EPOLLIN | EPOLLET;
    evt.data.fd = cfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &evt);
    if (ret == -1)
    {
        perror("epoll_ctl add cfd error:");
        exit(-1);
    }
    return;
}

/**
 * @description: Parsing http request data from browser
 * @param {*}
 * @return {*}
 */
void parse_request(int cfd, int epfd)
{
    char line[1024];
    char method[16], path[256], protocol[16];
    struct stat file_stat;
    int numbers = 0, len = 0;
    numbers = get_line(cfd, line, sizeof(line));
    if (numbers == 0){
        printf("client close\n");
        epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
        close(cfd);
    } else {
        sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
        /*judge whether this file exist*/
        printf("method:%s fileName:%s protocol:%s", method, path, protocol);
        decode_str(path, path);
        char *file = path + 1;
        if (strcmp(path, "/") == 0) {
            file = "./";
        }
        while (1)
        {
            char request_head[1024];
            len = get_line(cfd, request_head, sizeof(request_head));
            if (len == 0)
            {
                break;
            }
            // printf("%s", request_head);
        }
        if (strcasecmp(method, "GET") == 0) {
            handle_GET(cfd, file);
        } else if (strcasecmp(method, "POST") == 0) {
            handle_POST(cfd,file);
        } else {
            unimplemented(cfd);
            return ;
        }
    }

    return;
}
/**
 * @description: Epoll wait client and judge event
 * @param : listen fd
 * @return : void
 */
void epoll_listen(int lfd)
{
    int nready;
    int i;
    int sockfd;
    struct epoll_event evt, evts[OPENMAX];
    int epfd = epoll_create(OPENMAX);
    if (epfd == -1)
    {
        perror("epoll_create error");
        exit(-1);
    }
    evt.events = EPOLLIN;
    evt.data.fd = lfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &evt);
    while (1)
    {
        nready = epoll_wait(epfd, evts, OPENMAX, -1);
        if (nready == -1)
        {
            perror("epoll_wait error:");
            exit(-1);
        }
        for (i = 0; i < nready; ++i)
        {
            if (!(evts[i].events & EPOLLIN))
            {
                continue;
            }
            sockfd = evts[i].data.fd;
            if (sockfd == lfd)
            {
                deal_accept(lfd, epfd);
            }
            else
            {
                parse_request(sockfd, epfd);
            }
        }
    }
    close(lfd);
    close(epfd);
    return;
}
int main(int argc, char **argv)
{
    int lfd;
    u_short port;
    int ret = 0;
    if (argc == 1) {
        port = default_port;
        ret = chdir("/");
        if (ret == -1)
        {
            perror("chdir error");
            exit(-1);
        }
    } else if (argc == 2) {
        ret = chdir("/");
        if (ret == -1)
        {
            perror("chdir error");
            exit(-1);
        }
    } else {
        port = atoi(argv[1]);
        ret = chdir(argv[2]);
        if (ret == -1)
        {
            perror("chdir error");
            exit(-1);
        }
    }
    lfd = initSocket(port);
    epoll_listen(lfd);
    close(lfd);
    return 0;
}