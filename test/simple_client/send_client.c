#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define PORT 2233
#define IPV4 "127.0.0.1"

int main()
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		perror("socket");
		return 1;
	}
	
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	inet_pton(AF_INET, IPV4, &server_addr.sin_addr);

	if (connect(server_fd, (struct sockaddr*)&server_addr,
				sizeof(server_addr)) < 0)
	{
		perror("connect");
		return 1;
	}

	while(true)
	{

		char buf[BUFSIZ];
		if (fgets(buf, BUFSIZ, stdin) == NULL)
		{
			if (feof(stdin))
				return 0;
			perror("fgets");
			return 1;
		}
		
		if (write(server_fd, buf, strlen(buf)) < 0)
		{
			perror("write");
			return 1;
		}
	}
}
