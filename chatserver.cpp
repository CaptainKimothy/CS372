/***********************************************************************
 * Author: Kim McLeod
 * Date: 4.27.16
 * Filename: chatserver.cpp
 * Description: server side of a messaging system
 * ********************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>




#define BACKLOG 10     // number of allowed pending connections
#define MAXSIZE 500   // max size for messages
#define MAXHANDLESIZE 10 // max bytes allowed for handle




// get sockaddr
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}




// set up connection to client with port from argument
int setupConnect(char* portno){

    int sockfd, new_fd;  	// create sockets to listen on and send messages with
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int numbytes;



    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP



    if ((rv = getaddrinfo(NULL, portno, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through results and bind to first possible 
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }



    // servinfo no longer needed
    freeaddrinfo(servinfo); 



    // if p is NULL, the socket failed to bind
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connection\n");
        
	// accept connection
	sin_size = sizeof their_addr;
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	    
    close(sockfd);
	
	// if error with new socket, 
	if (new_fd == -1) {
		perror("accept");
		exit(1);
	}
	inet_ntop(their_addr.ss_family,
		get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
		
	return new_fd;
}




// function that allows client and server to communicate
void chat(int socket_fd, char* clientHandle){

	char buff[MAXSIZE];
	int numbytes;
	int quit;

	// while 
	while(1){
			
		memset(buff, 0, MAXSIZE);

		// if server didn't receive the data properly, error
		if ((numbytes = recv(socket_fd, buff, MAXSIZE-1, 0)) == -1) {
			perror("recv");
			exit(1);
		}

		// if the client quits, end connection
		if (strncmp(buff, "Client ended connection", 27) == 0){
			printf("%s\n", buff);
			close(socket_fd);
			exit(0);
		}


   	buff[numbytes-1] = '\0';
    	printf("%s> %s\n", clientHandle, buff);
    		
		// send message to client
		memset(buff, 0, MAXSIZE);
		printf("Server> ");
		fgets(buff, MAXSIZE-1, stdin);
		quit = strncmp(buff, "\\quit", 4);
		
		// if the server quits, close connection
		if (quit == 0){
			if(send(socket_fd, "Server ended connection\n", 28, 0) == -1){
				perror("send");
			}
			close(socket_fd);
			exit(0);
		}

		else{
			if(send(socket_fd, buff, strlen(buff), 0) == -1){
				perror("send");
			}
		}
  
	}
	return;
}




// calls functions and runs program
int main(int argc, char *argv[])
{
    int new_fd; 
    int numbytes;
    char cliHandle[MAXHANDLESIZE];
            
    if (argc != 2) {
        fprintf(stderr,"usage: server <portnumber>\n");
        exit(1);
    }
    
    //connect to Client
    new_fd = setupConnect(argv[1]);

	
	if ((numbytes = recv(new_fd, cliHandle, MAXSIZE-1, 0)) == -1) {
				perror("recv");
				exit(1);
	}	
	
	cliHandle[numbytes] = '\0';
	printf("server: connected to %s\n", cliHandle);
	printf("\nClient must sent first message\n");
	printf("To quit, enter '\\quit'\n\n");
	chat(new_fd, cliHandle); 

    return 0;
}
