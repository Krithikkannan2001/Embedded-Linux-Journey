#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main(void)
{

int sockfd = socket(AF_INET, SOCK_STREAM,0);
if(sockfd <0)
{
perror("Socket cration failed!\n");
exit(1);
}
printf("Socket Created Successfully, fd = %d\n",sockfd);


int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        exit(1);
    }


struct sockaddr_in addr;
memset(&addr,0,sizeof(addr));
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = INADDR_ANY;
addr.sin_port = htons(PORT);

if(bind(sockfd, (struct sockaddr *)&addr,sizeof(addr))<0)
{
perror("Bind Failed\n");
close(sockfd);
exit(1);
}
printf("Socket bound successfully to port %d\n",PORT);

if (listen(sockfd, 5) < 0) {
        perror("listen failed");
        close(sockfd);
        exit(1);
    }
    printf("Listening on port %d... waiting for a client to connect\n", PORT);

    int client_fd = accept(sockfd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept failed");
        close(sockfd);
        exit(1);
    }
    printf("Client connected! client_fd = %d\n", client_fd);

char buffer[256];
memset(buffer,0,sizeof(buffer));
int bytes_read = read(client_fd,buffer,sizeof(buffer) - 1);
if(bytes_read > 0)
{
printf("Recieved from Client\n%s\n",buffer);

char *response = "Hello from Raspberry Pi server!\n";
write(client_fd,response,strlen(response));
}

    close(client_fd);

close(sockfd);

return 0;

}
