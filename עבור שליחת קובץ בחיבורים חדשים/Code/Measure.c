
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>


#include <sys/time.h>
#include<stdio.h>
#include <time.h>



#define True 1
#define False 0

#define SERVER_PORT 5061
#define SERVER_IP "0.0.0.0"

#define TIMES 5 // Number of tests performed for each CC
#define FILESIZE 2000000 // Backup file size so if the program doesn't get the right size.


float timedifference_msec(struct timeval t0, struct timeval t1) // https://stackoverflow.com/questions/10192903/time-in-milliseconds-in-c
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

void recv_f(int s){
    char buffer[BUFSIZ];
    int file_size;
    int remain_data = 0;
    int n;
    recv(s, buffer, BUFSIZ, 0);
    file_size = atoi(buffer);
    if (file_size == 0)
        file_size = FILESIZE;
    printf("File size : %d\n", file_size);
    
    remain_data = file_size;
    int nextSize = BUFSIZ;
    while ((remain_data > 0) && ((n = recv(s, buffer, nextSize, 0)) > 0))
    {
            remain_data -= n;
            if (remain_data < BUFSIZ)
                nextSize = remain_data;
    }
}


double getSendAvg(int client, int iter){
    	double totalTime = 0;
    	struct timeval startTime, stopTime;
	gettimeofday(&startTime, NULL); // START: Time the file transfer
	recv_f(client); // Get data
	gettimeofday(&stopTime, NULL); // END: Time the file transfer
	printf("Recieved file no. %d\n", iter); // Aknowledge the user that he recieved the file
	totalTime += (double)timedifference_msec(startTime, stopTime); // Add the time difference to the totalTime

    	totalTime /= TIMES; // Now totalTime is avg time
    	return totalTime;
}

int main(){

    signal(SIGPIPE, SIG_IGN); // on linux to prevent crash on closing socket

    // Open the listening (server) socket
    int sock = -1;  

    char buf[256]; // Server TCP congestion control type
    socklen_t len;
    len = sizeof(buf);
    
    // print start time
    time_t cTime;
    time(&cTime);
    printf("Server tester started at %s", ctime(&cTime));
	 
    if((sock = socket(AF_INET , SOCK_STREAM , 0 )) == -1) // Creating server socket
    {
        printf("Could not create listening socket : %d",errno);
    }

    int enableResue = 1;
    if ((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enableResue, sizeof(int))) < 0){ // Make the socket reusable
        printf("setsockopt(..SO_REUSEADDR..) failed: %d \n", errno);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr)); // 0 padding serverAddr

    serverAddr.sin_addr.s_addr = INADDR_ANY; // Socket settings: protocol, convert to network byte-order etc.
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(sock, (struct sockaddr *)&serverAddr , sizeof(serverAddr)) == -1){ // Socket binding
        printf("Bind failed with error code : %d\n" , errno);
        close(sock);
        return -1;
    }
    printf("Server binded\n");
    

    if (listen(sock, 500) == -1){ // Listen for clients | 500 is the max size
        printf("Error listenning\n");
        close(sock);
        return -1;
    }



    printf("Waiting for TCP connections...\n");
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    // Accept client while using Cubic algo.
    int connections = 0;
    double cubicAvg = 0;
    while (connections < TIMES){
	    memset(&clientAddr, 0, sizeof(clientAddr));
	    clientAddrLen = sizeof(clientAddr);
	    int client = accept(sock, (struct sockaddr *)&clientAddr, &clientAddrLen);
	    // Check if client accepted
	    if (client == -1){
		printf("Failed accepting client\n");
		close(sock);
		close(client);
	    }
	    printf("Client accepted.\n");
	    // Get Congesion control type, technically we are just recieving data but still.
	    if (getsockopt(client, IPPROTO_TCP, TCP_CONGESTION, buf, &len) != 0) { 
		perror("getsockopt");
		close(client);
		close(sock);
		return -1;
	    } 
	    printf("\n\nUsing %s as TCP congestion controller\n", buf); 
	    // Print result using this type
	    cubicAvg += getSendAvg(client, connections);
	    close(client);
	    
	    connections ++;
    }
    cubicAvg = cubicAvg / 5;
    // Change congestion control type
    connections = 0;
    double renoAvg = 0;
    while (connections < TIMES){
	    // Accept client TESTS times, change algo to reno and print avg send time.
	    memset(&clientAddr, 0, sizeof(clientAddr));
		    clientAddrLen = sizeof(clientAddr);
		    int client = accept(sock, (struct sockaddr *)&clientAddr, &clientAddrLen);
		    // Check if client accepted
		    if (client == -1){
			printf("Failed accepting client\n");
			close(sock);
			close(client);
		    }
		    printf("Client accepted.\n");
		    // Get Congesion control type, technically we are just recieving data but still.
		    if (getsockopt(client, IPPROTO_TCP, TCP_CONGESTION, buf, &len) != 0) { 
			perror("getsockopt");
			close(client);
			close(sock);
			return -1;
		    } 
	    
	    strcpy(buf, "reno"); 
	    len = strlen(buf);
	    if (setsockopt(client, IPPROTO_TCP, TCP_CONGESTION, buf, len) != 0) {
		perror("setsockopt");
		close(client);
		close(sock); 
		return -1;
		}
	    len = sizeof(buf); 
	    if (getsockopt(client, IPPROTO_TCP, TCP_CONGESTION, buf, &len) != 0) {
		perror("getsockopt"); 
		close(client);
		close(sock); 
		return -1; 
		} 
	    printf("\n\nTCP congestion controller changed to %s\n", buf); 
	    
	    renoAvg += getSendAvg(client, connections);
	    connections ++;
    }
    renoAvg = renoAvg / 5;
    printf("Avg of cubic is %f ms\n", cubicAvg);
    printf("Avg of reno is %f ms\n", renoAvg);
    close(sock);
    return 0;
    

}
