#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#define PORT 2233
#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 4096

int main(void)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("socket");
		return 1;
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
	{
		perror("inet_pton");
		close(sockfd);
		return 1;
	}

	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("connect");
		close(sockfd);
		return 1;
	}

	char buf[BUF_SIZE];
	ssize_t n;
	printf("Waiting for message from server...\n");
	while ((n = read(sockfd, buf, sizeof(buf) - 1)) > 0)
	{
		buf[n] = '\0';
		printf("%s", buf);
	}
	if (n < 0)
	{
		perror("read");
	}
	else
	{
		printf("\nConnection closed by server.\n");
	}

	close(sockfd);
	return 0;
}
