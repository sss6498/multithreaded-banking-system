#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "server_helper.h"

int networkfd;

static void inputsignal( int signo ) {
  char command[5] = "exit";
	printf( "Received Signal SIGNIT: Sending exit command to Server before Client dies. . .\n");

  if ( (write(networkfd, command, strlen(command) ) ) < 0){
         printf("----- Sorry, your session has expired. -----\n");
  }
  return;
}

void * userserverfunction(void * args){
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int socketfd = *(int *)args;
    int status;

    while( (status = read(socketfd, buffer, sizeof(buffer)) ) > 0 ){
        printf("%s", buffer);
        memset(buffer, 0, sizeof(buffer));
    }
	printf("----- Closed, thank you! \n Connection closed \n");
    close(socketfd);
    free(args);
    return 0;
}

int main(int argc, char *argv[]){

    char buffer[256];
    int socketfd;
    void *socket_arg1 = malloc(sizeof(socketfd));
    void *socket_arg2 = malloc(sizeof(socketfd));

    struct addrinfo request;
    request.ai_flags = 0;
    request.ai_family = AF_INET;
    request.ai_socktype = SOCK_STREAM;
    request.ai_protocol = 0;
    request.ai_addrlen = 0;
    request.ai_canonname = NULL;
    request.ai_next = NULL;
    struct addrinfo *result;

    if(argc < 2){
        printf("ERROR: Specify server\n");
        exit(1);
    }

    if ( gethostbyname(argv[1]) == NULL){
        printf("ERROR: wrong Host\n");
        exit(1);
    }

    getaddrinfo(argv[1], "9454", &request, &result );

    socketfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    memcpy(socket_arg1, &socketfd, sizeof(int));
    memcpy(socket_arg2, &socketfd, sizeof(int));

    int status = connect(socketfd, result->ai_addr, result->ai_addrlen);
    while( status < 0 ){
        close(socketfd);
        socketfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if(errno != ECONNREFUSED){
              printf("ERROR: %s\n", strerror(errno));
              exit(1);
        }
        printf("Server not found. Reconnecting in 3 seconds...\n");
        sleep(3);
        status = connect(socketfd, result->ai_addr, result->ai_addrlen);
    }

    networkfd = socketfd;
    struct sigaction	action;
    action.sa_flags = 0;
	  action.sa_handler = inputsignal;
	  sigemptyset( &action.sa_mask );
	  sigaction( SIGINT, &action, 0 );

    printf(" -----Connected!-----\n");

    pthread_t server_user;

    if (pthread_create(&server_user, NULL, &userserverfunction, socket_arg2 ) != 0){
         printf("ERROR: cannot create thread: %s\n", strerror(errno));
         exit(1);
    }

    pthread_detach(server_user);
    memset(buffer, 0, sizeof(buffer));

    while( read(0, buffer, sizeof(buffer)) > 0) {
        if ( (status = write(socketfd, buffer, strlen(buffer) ) ) < 0){
               printf("----- Session expired -----\n");
               break;
        }
        memset(buffer, 0, sizeof(buffer));
        sleep(2);
    }
    return 0;
}
