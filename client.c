#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct {
    int x1, y1, x2, y2;
} Line;

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_pton(AF_INET, "172.23.3.212", &server_addr.sin_addr);

    connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    Line line;
    // 这里模拟发送一条线
    line.x1 = 10; line.y1 = 10; line.x2 = 100; line.y2 = 100;
    send(sockfd, &line, sizeof(line), 0);

    close(sockfd);
    return 0;
}