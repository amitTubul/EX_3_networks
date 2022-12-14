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

int main(){

    int sock ,new_sock;
    struct sockaddr_in reciver_addr , new_addr;
    socklen_t addr_size;

    sock= socket(AF_INET, SOCK_STREAM, 0);//creates socket descriptor for TCP ipv4 connection

    if (sock == -1) {
        printf("could not create a socket");
        return -1;
    }
    printf("socket created\n");

    reciver_addr.sin_family = AF_INET;//family address of reciver_address is AF_INET
    reciver_addr.sin_port = RECIVER_PORT;//port address of reciver_addr is 5555 in "big endian"
    reciver_addr.sin_addr.s_addr= inet_addr(RECIVER_IP);

    int bindresult= bind(sock,(struct sockaddr *)&reciver_addr,sizeof (reciver_addr));
    if(bindresult==-1){
        printf("bind faild \n");
        close(sock);
        return -1;
    }
    printf("bind() succeed\n");

    int listenresult= listen(sock,1);
    if(listenresult==0){
        printf("listening....\n");
    }
    if (listenresult==-1){
        printf("listen has failed \n");
        close(sock);
        return -1;
    }
    printf("waiting for incoming tcp connection\n");

    addr_size=sizeof (new_addr);

    new_sock = accept(sock, (struct sockaddr *)&new_addr, &addr_size);
    if (new_sock == -1) {
        printf("listening failed\n");
        close(sock);
        return -1;
    }
    printf("a new sender connection accepted\n");


    char buffer[BUFFER_SIZE];
    FILE *f;

    f= fopen("textRecived.txt","w");
    if(f==NULL){
        printf("error in creating file");
    }

    int byte_recived;

    while (1) {
        byte_recived = recv(new_sock, buffer, BUFFER_SIZE, 0);
        if (byte_recived == -1) {
            printf("recv() had failed\n");
            break;
        }
        if (byte_recived == 0) {
            printf("error, connection been closed \n");
            break;
        }

        fprintf(f, "%s", buffer);
        bzero(buffer, BUFFER_SIZE);
    }
    printf("data has recived and written into a file\n");

    return 0;
}










