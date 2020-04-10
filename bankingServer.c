 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <string.h>
 #include <errno.h>
 #include <ctype.h>

 #include <sys/types.h>
 #include <sys/socket.h>

 #include <netinet/in.h>
 #include <netdb.h>

 #include <pthread.h>

 #include "server_helper.h"

 account **arr_account;
 int numAccount;
 pthread_mutex_t openAuthLock;



 void * clientServiceFunction(void * arg){
      printf("Server has just accepted a client connection.\n");
      int client_socket_fd = *(int *) arg;

      char command[10];
      char name[100];
      char ibuffer[256];
      char obuffer[256];

      memset(ibuffer, 0, sizeof(ibuffer));
      memset(obuffer, 0, sizeof(obuffer));
      memset(command, 0, sizeof(command));
      memset(name, 0, sizeof(name));

      command_state cs;


      int session = -1;

      char c_options[100] = "COMMANDS:\n\tcreate\n\tserve\n\tdeposit\n\twithdraw\n\tquery\n\tend\n\tquit";
      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "%s\n",c_options) ) < 0 ){
          printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
          exit(1);
        }
      memset(obuffer, 0, sizeof(obuffer));
     
      while( read(client_socket_fd, ibuffer, 255) > 0){
          sscanf(ibuffer, "%s %s",command, name);

          cs = commandchoice(command);

          switch(cs){
              case cs_create:
                  accountcreate(client_socket_fd,&numAccount,&arr_account,name,&session,&openAuthLock);
                  break;
              case cs_serve:
                  accountserve(client_socket_fd,&arr_account,name,&session);
                  break;
              case cs_deposit:
                  accountdeposit(client_socket_fd,&arr_account,name,&session);
                  break;
              case cs_withdraw:
                  accountwithdraw(client_socket_fd,&arr_account,name,&session);
                  break;
              case cs_query:
                  accountquery(client_socket_fd,&arr_account,&session);
                  break;
              case cs_end:
                  accountend(client_socket_fd,&arr_account,&session);
                  break;
              case cs_quit:     
                  if(session < 0){
                      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "- - - EXITED - - -\n- - -Thank you. Please come again! - - -.\n") ) < 0 ){
                          printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
                          exit(1);
                        }
                      printf("Server has closed a client connection\n");
                      close(client_socket_fd);
                      free(arg);
                      return 0;
                    }
                  
                    arr_account[session]->service_flag = 0;
                    pthread_mutex_unlock(&(arr_account[session]->lock));
                    session = -1;
                      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "- - - EXITED - - -\n- - -Thank you. Please come again! - - -.\n") ) < 0 ){
                          printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
                          exit(1);
                        }
                    printf("Server has closed a client connection\n");
                    close(client_socket_fd);
                    free(arg);
                  return 0;
              default:
                  if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Invalid argument: %s\n",command) ) < 0 ){
                      printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
                      exit(1);
                  }
                  break;

          }
          if ( write(client_socket_fd, obuffer, sprintf(obuffer, "%s\n",c_options) ) < 0 ){
              printf("ERROR: WRITE FAILED: %s\n", strerror(errno));
              exit(1);
            }

          memset(ibuffer, 0, sizeof(ibuffer));
          memset(obuffer, 0, sizeof(obuffer));
          memset(command, 0, sizeof(command));
          memset(name, 0, sizeof(name));
      }

      free(arg);
      return 0;
 }

 void *diagnostic(void * args){

      while(1){
          printf("BANK INFORMATION: \n");
          if(numAccount == 0){
              printf("\tNo Account have been opened yet\n");
          }else{

              int i;
              for(i = 0 ; i < 1000 ; i++){
                  account * ca = arr_account[i];
                  if(ca->service_flag >= 0){
                      if(ca->service_flag == 0){
                          printf("Account name: %s, Balance: %f, In Service: %s\n",ca->name,ca->balance,"NO");
                      }else{
                          printf("Account name: %s, Balance: %f, In Service: %s\n",ca->name,ca->balance,"YES");
                      }
                  }
              }
          }
          sleep(15);
      }
 }

 int main() {

    int server_socket_fd;

    int client_socket_fd;

    struct addrinfo request;
    request.ai_flags = AI_PASSIVE;
    request.ai_family = AF_UNSPEC;
    request.ai_socktype = SOCK_STREAM;
    request.ai_protocol = 0;
    request.ai_addrlen = 0;
    request.ai_canonname = NULL;
    request.ai_next = NULL;

    struct addrinfo *result;


    getaddrinfo(0, "9454", &request, &result );


    if ( (server_socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol) ) < 0 ){
        printf("ERROR: SERVER COULD NOT BE CREATED: %s\n", strerror(errno));
        exit(1);
    }


    if( bind(server_socket_fd, result->ai_addr, result->ai_addrlen) < 0){
        printf("ERROR: SERVER COULD NOT BE CREATED: %s\n", strerror(errno));
        exit(1);
    }
    int optval = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR , &optval, sizeof(int));

    numAccount = 0;
    initialize(&arr_account);
    pthread_mutex_init(&openAuthLock, NULL);



    pthread_t get_account_info;
    pthread_create(&get_account_info,NULL,&diagnostic,NULL);

    listen(server_socket_fd,5); 
    void * client_socket_arg;   
    pthread_t client;
    while(1){

        if( (client_socket_fd = accept(server_socket_fd, NULL, NULL)) < 0){
            printf("ERROR: FAILED TO ACCEPT: %s\n", strerror(errno));
            exit(1);
        }

        client_socket_arg = malloc(sizeof(int));
        memcpy(client_socket_arg, &client_socket_fd, sizeof(int));


        if (pthread_create(&client, NULL, &clientServiceFunction, client_socket_arg ) != 0){
            printf("ERROR: Can't create user server thread: %s\n", strerror(errno));
            exit(1);
        }

        if (pthread_detach(client) != 0){
            printf("ERROR: Could not detach client thread: %s\n", strerror(errno));
            exit(1);
        }



    }
}
