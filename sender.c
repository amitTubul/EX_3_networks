#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>



#define RECIVER_IP "10.0.2.15"
#define RECIVER_PORT 5555
#define BUFFER_SIZE 1024

int main () {

    struct sockaddr_in reciver_addr;
    int sock;

    sock= socket(AF_INET, SOCK_STREAM, 0);//creates socket descriptor for TCP ipv4 connection
    if (sock == -1) {
        printf("could not create a socket");
        return -1;
    }
    printf("socket created\n");


    reciver_addr.sin_family = AF_INET;//family address of reciver_address is AF_INET
    reciver_addr.sin_port = RECIVER_PORT;//port address of reciver_addr is 5555 in "big endian"
    reciver_addr.sin_addr.s_addr= inet_addr(RECIVER_IP);


    int connection_check=connect(sock,(struct sockaddr *)&reciver_addr , sizeof (reciver_addr));
    if(connection_check==-1) {
        printf("connect() had failed\n");
        close(sock);
        return -1;
    }
    printf("connected to reciver\n");

    FILE *f=fopen("sendfile.txt","r");
    if(f==NULL){
        printf("error in reading file\n");
        return-1;
    }

    char data[BUFFER_SIZE] = {0};
    int send_check;

    while(fgets(data, BUFFER_SIZE, f) != NULL) {
        send_check=send(sock, data, BUFFER_SIZE, 0);
        if (send_check == -1) {
            printf("send() had failed\n");
            return-1;
            }
        if (send_check == 0) {
            printf("error, connection been closed \n");
            return-1;
        }
        bzero(data, BUFFER_SIZE);
    }

    printf("file 'senderfile' successfully sent\n");
    printf("closing socket\n");

    close(sock);

    return 0;
}