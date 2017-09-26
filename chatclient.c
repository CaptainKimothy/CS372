/***********************************************************************
 * Author: Kim McLeod
 * Date: 4.27.16
 * Filename: chatclient.c
 * Description: client side of a messaging system
 * ********************************************************************/




#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
 



#define MAXSIZE 500 // max size for messages
#define MAXHANDLESIZE 10 // max bytes allowed for handle





// get sockaddr
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}





// set up connection with server with host and 
// port number from argument
int setupConnect(char* hostname, char* portno){
	
    int sockfd;
    char buf[MAXSIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN]; 
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, portno, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through results and connect to first possible
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

	// if problem with connecting, send error
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(1);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

   // servinfo is no longer needed
    freeaddrinfo(servinfo);
	
	return sockfd;
}



// enables messaging between client and server
void chat(int socket_fd, char* handle){

	char buff[MAXSIZE];
	int numbytes;
	int quit;
	while(1){
	
		// send
		memset(buff, 0, MAXSIZE);
		printf("%s> ", handle);
		fgets(buff, MAXSIZE-1, stdin);
		quit = strncmp(buff, "\\quit", 4);
		

		// if client quits, close connection
		if (quit == 0){
			if(send(socket_fd, "Client ended connection\n", 28, 0) == -1){
				perror("send");
				exit(1);
			}
			close(socket_fd);
			exit(0);
		}


		else{
			if(send(socket_fd, buff, strlen(buff), 0) == -1){
				perror("send");
				exit(1);
			}
		}
		

		// receive message from server
		if ((numbytes = recv(socket_fd, buff, MAXSIZE-1, 0)) == -1) {
			perror("recv");
			exit(1);
		}


		// if server closes connection
		if (strncmp(buff, "Server ended connection", 27) == 0){
			printf("%s\n", buff);
			close(socket_fd);
			exit(0);
		}

		buff[numbytes-1] = '\0';
		printf("Server> %s\n",buff);

	}
	return;
}



// calls functions and controls flow of program
int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    char buf[MAXSIZE];
    char handle[MAXHANDLESIZE];
    size_t ln;
    
    if (argc != 3) {
        fprintf(stderr,"usage: client hostname portnumber\n");
        exit(1);
    }

	//connect to Server
	sockfd = setupConnect(argv[1], argv[2]);

	// prompt for handle
	printf("Please enter a handle up to %d characters: ", MAXHANDLESIZE);
	fgets(handle, MAXHANDLESIZE, stdin);
	ln = strlen(handle) - 1;
	
	if (handle[ln] == '\n')
    	{    handle[ln] = '\0';  }
   
 	if(send(sockfd, handle, strlen(handle), 0) == -1){
		perror("send");
		exit(1);
	}
	
	// start chat
	printf("\nReady to start chat\n");
	printf("To quit, enter '\\quit'\n\n");
	chat(sockfd, handle);      


    // close connection
    close(sockfd);

    return 0;
}
