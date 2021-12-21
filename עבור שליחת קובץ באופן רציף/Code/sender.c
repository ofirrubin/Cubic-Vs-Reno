#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/sendfile.h>

#include <stdio.h>
#include <time.h>

#define IP "127.0.0.1"
#define PORT 5062
#define PSIZE 65535
#define FILENAME "file.txt"

#define TESTSIZE 5


int sendf(FILE *f, int sock, int pos){ // Sends file f using socket sock, notify iter num -> <pos> on fail.
    char data[PSIZE] = {0};
    while(fgets(data, PSIZE, f) != NULL)
    {
        if (send(sock, data, PSIZE, 0) == -1) // sizeof(data) == PSIZE
        {
            printf("Error sending file %d / %d => %d", pos, TESTSIZE, errno);
            return -1;
        }
    }
    return 0;
}

int sendMyfile(FILE *f, int sock, int pos){
    /* Get file size */
    rewind(f);
    fseek(f, 0L, SEEK_END);
    int fileSize = ftell(f);
    char fileS[256] = {0};
    rewind(f);
    // Convert file size to char array so we can send it over socket.
    sprintf(fileS, "%d", fileSize);
    send(sock, fileS, sizeof(fileS), 0); // Send file size.


    int remain_data = fileSize;
    int sent_bytes;
    /* Sending file data */
    while (((sent_bytes = sendfile(sock, fileno(f), NULL, BUFSIZ)) > 0) && (remain_data > 0))
    {
        remain_data -= sent_bytes;
    }
    return 0;
}

void sendFtimes(FILE *f, int sock){ // Sends the file TESTSIZE times.
     int iter = 0; // in test iter
    
    while(iter < TESTSIZE)
    {
        int bytesSent = sendMyfile(f, sock, iter);
        if (0 == bytesSent)
            printf("File no. %d / %d Successfully sent.\n", ++iter, TESTSIZE);
    }
    printf("Test completed\n");
}

int main(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    FILE *f;
    char *fname = FILENAME;

    char buf[256]; // Server TCP congestion control type
    socklen_t len;
    len = sizeof(buf);

    time_t cTime;
    time(&cTime);
    printf("Client tester started at %s\n", ctime(&cTime));
 

    struct sockaddr_in serverAddress; // Set serverAddress to all zeros.
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET; // Init. socket type
    serverAddress.sin_port = htons(PORT); // htons translates host byte-order (might var.) to the network byte-order (TCP -> big endian)

	int rval = inet_pton(AF_INET, (const char*)IP, &serverAddress.sin_addr); // Converts the address (text) to binary

	if (rval <= 0)
	{
		printf("Failed converting server address to binary.");
		return -1;
	}

     // Make a connection to the server with socket SendingSocket.

     if (connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1)
     {
	   printf("Error connecting to the server: %d\n",errno);
       return -1;
     }

     printf("connected to server\n");

    if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, &len) != 0) { // Get TCP congestion control algorithm name into buf.
        perror("getsockopt(..TCP_CONGESTION..buf,..) error \n");
        close(sock);
        return -1;
    } 
    printf("\n\nUsing %s as TCP congestion controller\n", buf); 

     // Sends some data to server
     f = fopen(fname, "r");
     if (f == NULL){
        printf("Error reading from file\n");
        close(sock);
        return -1;
     }

    sendFtimes(f, sock);

    // Change congestion control type
    strcpy(buf, "reno"); 
    len = strlen(buf);
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, len) != 0) { // Replace algo
        perror("setsockopt(..TCP_CONGESTION.., buf..) error\n");
        close(sock); 
        return -1;
    }
    len = sizeof(buf); 
    if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, &len) != 0) { // Confirm replace
        perror("getsockopt(..TCP_CONGESTION..buf,..) error \n");
        close(sock); 
        return -1; 
        } 
    
    printf("\n\nTCP congestion controller changed to %s\n", buf); 
    sleep(10);
    sendFtimes(f, sock);

    close(sock); // Close the connection

    return 0;
}
