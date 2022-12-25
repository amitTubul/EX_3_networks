#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>


//function that send the file to the receiver
int send_file(int sock,char* data,int size){

    int sentbytes=send(sock,data,size,0);
    if(sentbytes==-1)
    {
        perror("Text didnt sent\n");
        return -1;
    }
    else if(sentbytes==0)
    {
        printf("The TCP connection has closed before the send()\n");
    }
    else if(sentbytes<size)
    {
        printf("Sent only %d ot of %d bytes of the text.\n",sentbytes,size);
    }
    else
    {
        printf("Full massage sent succssesfully.\n");
    }
    return sentbytes;
}

//checking the authentication of the massage
//we calculated the xor already
int AuthenticationCheck(int sock)
{
    char* authentication_check="10100001000";
    char check[12];

    recv(sock,&check,sizeof(check),0);

    if(strcmp(authentication_check, check) != 0){
        printf("The receiver didnt get the message that the sender sent\n");
        return 0;
    }
    else
    {
        printf("The receiver get the message completely that the sender sent\n");
        return 0;
    }

}

int main()
{
    //step 1:
    //that part is to open the file and read it into a char array
    FILE *pointer_to_file=fopen("test.txt","r");
    if(pointer_to_file==NULL)
    {
        perror("unable to read file\n");
        return -1;
    }
    //fseek() is used to move file pointer associated with a given file to a specific position.
    //using fseek() we moving file pointer to the end 
    //then using ftell(), we find its position which it actually size in bytes
    fseek(pointer_to_file,0l,SEEK_END);

    //calculating the size of the file
    int Size_of_file=ftell(pointer_to_file);
    printf("The amout of bytes of the file is: %d\n",Size_of_file);

    //split the size in half for the 2 parts
    int half_size_of_file=Size_of_file/2;

    fseek(pointer_to_file,0L,SEEK_SET);

    //read data from the file and copy it to the char array
    char file_data[Size_of_file];
    fread(file_data,Size_of_file,sizeof(char),pointer_to_file);

    //close the file
    fclose(pointer_to_file);
    printf("read file successfully\n");

    //step 2:
    //the ip and the port that intended for the socket creation
    char* ip="127.0.0.1";
    int port=5556;

    //creating the TCP sender socket descriptor
    int sock=socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("Error in socket\n");
        return -1;
    }
    printf("TCP socket descriptor created successfully.\n");

    //defining a ipv4 internet address struct that intended to make
    // the connection between the sender and receiver
    //the ip and the port are going to be converted into binary numbers using inet_pton() for the ip
    // and htons() for the port in big endian
    //memset function deletes n first chars and replace it by 0
    //sin_family is to identify the address as ipv4
    struct sockaddr_in receiver_addr;
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);

    int addr_bin_conv=inet_pton(AF_INET, ip , &(receiver_addr.sin_addr));
    if(addr_bin_conv<=0){
        perror("Error in change ip to binary\n");
        return -1;
    }

    //establishing the connection with the receiver
    int connection_request= connect(sock, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr));
    if (connection_request == -1)
    {
        perror("Error in the connection\n");
        return -1;
    }
    printf("Connected successfully.\n");

    //sending to the receiver the size of the file in bytes to know how much he need to received for each half of the file;
    int send_size_file=Size_of_file;
    send(sock,&send_size_file,sizeof(int),0);
    printf("sending to the receiver the size of the file.\n ");

    //sending the 2 messages and changing the cc algorithm every iterate until we tell to exit
    while(1)
    {
        char buffer[8];
        bzero(buffer,sizeof(buffer));

        //part 3:
        //sending the first half of the file
        printf("sending part 1 of the text\n");
        send_file(sock,file_data,(half_size_of_file));

        //part 4:
        //checking authentication of the first half of the text
        printf("Authentication check\n");
        AuthenticationCheck(sock);

        //part 5:
        //checking what cc algorithm we using and changing to another cc algo
        //getsockopt() is a function that tells us what cc algorithm the socket uses
        //setsockopt() is a function that change the cc algorithm for the socket
        socklen_t len=sizeof(buffer);
        if(getsockopt(sock,IPPROTO_TCP,TCP_CONGESTION,buffer,&len)!=0)
        {
            perror("getsockopt failed\n");
            return -1;
        }

        printf("The current CC algo: %s\n",buffer);

        strcpy(buffer,"reno");
        len=strlen(buffer);

        if(setsockopt(sock,IPPROTO_TCP,TCP_CONGESTION,buffer,len)!=0)
        {
            perror("setsockopt failed\n");
            return -1;
        }

        len=sizeof(buffer);

        if(getsockopt(sock,IPPROTO_TCP,TCP_CONGESTION,buffer,&len)!=0)
        {
            perror("getsockopt failed\n");
            return -1;
        }

        printf("after changing, the new CC algo: %s\n",buffer);

        //part 6:
        //start sending the part 2 with the new cc algorothm
        printf("part 2\n");
        printf("sending part 2 of the text\n");
        send_file(sock,(file_data+(half_size_of_file)-1),half_size_of_file);

        //checking with the receiver if he received the whole file completely
        int ack=1;
        printf("Waiting for OK signal.\n");
        int ack_got;
        recv(sock,&ack_got,sizeof(int),0);
        if(ack_got==ack)
        {
            printf("Sync-OK\n");
        }
        else
        {
            printf("The receiver didnt got the whole file.\n");
        }
        //part 7:
        //user decision: send the file again (Y) or exit (E)
        char c;
        printf("To send the file again please enter Y, to exit please enter E:\n");
        scanf(" %c",&c);

        while(c != 'Y' && c != 'E')
        {
            printf("There is no such option, please enter again Y/E:\n");
            scanf(" %c",&c);
            if(c == 'Y' || c == 'E'){
                break;
            }
        }

        //send_if_yes gets the value 1 if the sender decided to send the file again
        //then we send the send_if_yes to the receiver so the receiver will know that if he got the value 1 so he need to continue running the while loop again
        //and send_if_exit get the value 0 if the sender decided to exit
        //then we send the send_if_exit to the receiver so the receiver will know that if he got the value 0 he will know that the sender decide to exit and he will exit too

        //if the decision is yes (send the file again)
        if(c == 'Y')
        {
            //send the send_if_yes with the value 1
            int send_if_yes=1;
            send(sock,&send_if_yes,sizeof(int),0);

            //change back to cc algo cubic
            printf("changing back the CC algo to cubic.\n");
            strcpy(buffer,"cubic");

            if(setsockopt(sock,IPPROTO_TCP,TCP_CONGESTION,buffer,strlen(buffer))!=0)
            {
                perror("setsockopt failed\n");
                return -1;
            }
        }
        else
        {
            //send the send_if_exit with the value 0
            int send_if_exit=0;
            send(sock,&send_if_exit,sizeof(int),0);

            printf("exiting...\n");
            break;
        }

    }
    //closing the sender socket
    close(sock);

    printf("Connection closed.\n");

    return 0;
}
