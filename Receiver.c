#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/time.h>


int main(){

    int port=5556;

    //step 1:
    //creating the TCP receiver socket descriptor
    int listen_socket=socket(AF_INET,SOCK_STREAM,0);
    if(listen_socket==-1)
    {
        perror("Error in socket\n");
        return -1;
    }
    printf("TCP socket descriptor created successfully.\n");

    //defining a ipv4 internet address struct that intended to make
    //the connection between the sender and receiver
    //the ip and the port are going to be converted into binary numbers
    //using htons() for the port in big endian
    //memset function deletes n first chars and replace it by 0
    //sin_family is to identify the address as ipv4
    struct sockaddr_in receiver_addr;
    memset(&(receiver_addr),0,sizeof(receiver_addr));
    receiver_addr.sin_family=AF_INET;
    receiver_addr.sin_port=htons(port);
    receiver_addr.sin_addr.s_addr=INADDR_ANY;

    //to enable reuse with this port, we do this:
    int yes=1;
    if(setsockopt(listen_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1)
    {
        perror("setsockopt failed\n");
        return -1;
    }

    //bind the senders' ip and port for the TCP connection
    int bind_check = bind(listen_socket , (struct sockaddr*)&receiver_addr,sizeof(receiver_addr));
    if(bind_check==-1){
        perror("Error in binding\n");
        return -1;
    }
    printf("bind successfully\n");

    //waiting for incoming connections by listen() function
    if(listen(listen_socket,5)==-1){
        perror("Error in listening\n");
        return -1;
    }
    printf("Listening...\n");

    //defining the sender ipv4 address struct to save the senders data
    struct sockaddr_in sender_addr;
    socklen_t sender_len= sizeof(sender_addr);
    memset(&sender_addr,0,sizeof(sender_addr));

    //step 2:
    //accepting the senders' connect() request.
    //When a sender connects, the method returns a new socket object representing the connection.
    //so that our new socket contains the senders socket data
    int sender_socket=accept(listen_socket,(struct sockaddr*)&sender_addr,&sender_len);
    if(sender_socket==-1)
    {
        perror("Error in accept\n");
        return -1;
    }
    printf("Connection accepted!\n");
    
    //receiving from the sender the size of the file that we will know how much we need to recv in each half
    int file_size_from_sender;
    recv(sender_socket,&file_size_from_sender,sizeof(int),0);
    printf("The size of the file is: %d\n",file_size_from_sender);


    //define 2 arrays that will save the time of each half
    //the size of the arrays is 1000 because we assume that we cant ask more then 1000
    //requests (sent mail about it, that we can assume that)

    double time_of_first_half[1000];
    bzero(time_of_first_half,sizeof(time_of_first_half));
    double time_of_second_half[1000];
    bzero(time_of_second_half,sizeof(time_of_second_half));

    //counts the amount of time that the file received
    int count_times=0;

    //we start a loop to receive the 2 part of the file every iterate until the sender tells to exit

    int stillRunning=1;
    while(stillRunning==1){

        int total_bytes_received=0;
        char bufferReply[file_size_from_sender/2];
		bzero(bufferReply,sizeof(bufferReply));

        //the first time we create the socket his defult cc algo is cubic(so in the first time we enter the loop we dont need to change his cc algo to cubic)
        //but after this, every iterate we change back from reno to cubic
        //(the change from reno to cubic is before the end of the while loop, that it will happend if the user decided to send the file again)
        printf("now the cc algorithm is cubic.\n");
        printf("Ready to receive the first half of the file.\n");

        //measure the time of receive of the first half of the file
        struct timeval before_recv_time_half1, after_recv_time_half1;
        gettimeofday(&before_recv_time_half1,NULL);
        
        //step 3:
        //receiving the data from the sender until we get the full amount of the first half
        while(total_bytes_received<(file_size_from_sender/2))
        {
            int ReceivedBytes=0;
            ReceivedBytes=recv(sender_socket,bufferReply,sizeof(bufferReply),0);
            total_bytes_received+=ReceivedBytes;
            //checking if there is no exception with the ReceivedBytes bytes
            // if its dont -1 or 0 its mean the we got amount of bytes of the first half of the file from the sender
            if(ReceivedBytes==-1)
            {
                perror("recv failed\n");
                return -1;
            }
            else if(ReceivedBytes==0)
            {
                printf("Sender exited, exiting...\n");
				stillRunning = 0;
                break;
            }
        }

		if (stillRunning == 0)
        {
			break;
        }


        //measure the time that the massage was arrived
        gettimeofday(&after_recv_time_half1,NULL);
        
        //step 4:
        double seconds=(double) (after_recv_time_half1.tv_sec - before_recv_time_half1.tv_sec);
        double microseconds=(double) (after_recv_time_half1.tv_usec - before_recv_time_half1.tv_usec);

        //step 5:
        time_of_first_half[count_times]=seconds+ microseconds*1e-6;

        printf("The first part received.\n");

        //step 6:
        //sending the authentication to the sender again to confirm that the
        //first half of the file received completely
        //in authentication_check we calculated the xor already
        char* authentication_check="10100001000";

        int authentication_send=send(sender_socket,authentication_check,strlen(authentication_check),0);

        if(authentication_send > 0)
        {
            printf("sent ACK to the sender.\n");
        }

        printf("the first half of the file received completely.\n");

        //changing the CC algo from cubic in to reno
        char changeCC_buffer[8];
        bzero(changeCC_buffer,sizeof(changeCC_buffer));
        strcpy(changeCC_buffer,"reno");
        int len=strlen(changeCC_buffer);

        if(setsockopt(listen_socket,IPPROTO_TCP,TCP_CONGESTION,changeCC_buffer,len) != 0)
        {   
            perror("setsockopt failed\n");
            return -1;
        }

        printf("now the cc algorithm is reno\n");

        total_bytes_received=0;
        char bufferReply2[file_size_from_sender/2];
		bzero(bufferReply2,sizeof(bufferReply2));

        //mesure the time of receive the second half of the file
        struct timeval before_recv_time_half2,after_recv_time_half2;
        

        //step 7:
        //receiving the second half of the file
        printf("Ready to receive the second half of the file.\n");

        gettimeofday(&before_recv_time_half2, NULL);

        while(total_bytes_received<(file_size_from_sender/2))
        {
            int ReceivedBytes=0;
            ReceivedBytes=recv(sender_socket,bufferReply2,sizeof(bufferReply2),0);
            total_bytes_received+=ReceivedBytes;
            
            //checking if there is no exception with the ReceivedBytes bytes
            // if its dont -1 or 0 its mean the we got amount of bytes of the first half of the file from the sender
            if(ReceivedBytes==-1)
            {
                perror("Error with recv \n");
                return -1;
            }
            else if(ReceivedBytes==0)
            {
                printf("Sender exited, exiting...\n");
				stillRunning = 0;
                break;
            }
        }

		if (stillRunning == 0)
        {
			break;
        }      


        gettimeofday(&after_recv_time_half2 , NULL);
        
        //step 8:
        seconds=(double) (after_recv_time_half2.tv_sec - before_recv_time_half2.tv_sec);
        microseconds=(double) (after_recv_time_half2.tv_usec - before_recv_time_half2.tv_usec);

        //step 9:
        time_of_second_half[count_times]=seconds+ microseconds*1e-6;

        count_times++;

		
		printf("Second part received.\n");

        //sending a ack for receiving the full file 
		send(sender_socket, &stillRunning, sizeof(int), 0);

		printf("Sync packet sent.\n");

        //step 10:
        //receiving the resending status
        int run_or_not;
        recv(sender_socket,&run_or_not,sizeof(int),0);
        if(run_or_not==0){
            printf("Sender exited, exiting...\n");
            stillRunning=0;
            break;
        }
        //if the user decided to continue the program and send the file again
        //changing back cc algo from reno to cubic if the sender decide to continue
        //the program(the defult cc aglo that the socket have is cubic)
        bzero(changeCC_buffer,sizeof(changeCC_buffer));
        strcpy(changeCC_buffer,"cubic");
        len=strlen(changeCC_buffer);

        if(setsockopt(listen_socket,IPPROTO_TCP,TCP_CONGESTION,changeCC_buffer,len) != 0)
        {   
            perror("setsockopt failed\n");
            return -1;
        }
    }

    //closing the sender socket and the listen socket
	close(sender_socket);
    close(listen_socket);


    //printing the time took to each half in each iterate
    for(int i=0;i<count_times;i++){
        printf("Run %d of first half using cubic: %lf seconds.\n", i+1 , time_of_first_half[i]);
        printf("Run %d of second half using reno: %lf seconds.\n", i+1 , time_of_second_half[i]);
    }
    //calculating the sum of time for each half sent
    double sum_half_1=0;
    double sum_half_2=0;

    for(int i=0;i<count_times;i++){
        sum_half_1+=time_of_first_half[i];
        sum_half_2+=time_of_second_half[i];
    }

    //calculating time average for each half and for the while file
    double average_half_1=sum_half_1/count_times;
    double average_half_2=sum_half_2/count_times;
    double average_full_file= (sum_half_1+sum_half_2)/(2*count_times);


    printf("The average time for the first half: %lf seconds.\n",average_half_1);
    printf("The average time for the second half: %lf seconds.\n",average_half_2);
    printf("The average time of the full file is : %lf\n",average_full_file);

    
    printf("connection closed.\n");

    return 0;
}