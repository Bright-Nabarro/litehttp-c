#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 2323
#define BUF_SIZE    4096

int main(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    // 构造并发送HTTP GET请求
    const char *http_get =
        "GET / HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n"
        "\r\n";
    ssize_t sent = send(sockfd, http_get, strlen(http_get), 0);
    if (sent < 0) {
        perror("send");
        close(sockfd);
        return 1;
    }

    // 接收一次响应
    char buf[BUF_SIZE];
    ssize_t n = recv(sockfd, buf, sizeof(buf) - 1, 0);
    if (n < 0) {
        perror("recv");
        close(sockfd);
        return 1;
    } else if (n == 0) {
        printf("No response received, connection closed by server.\n");
        close(sockfd);
        return 1;
    }
    buf[n] = '\0';
    printf("Response from server:\n%s\n", buf);

    close(sockfd);
    return 0;
}
