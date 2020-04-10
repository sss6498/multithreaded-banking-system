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


int isamount(char * amount){
    int periodCount = 0;
    int len = strlen(amount);
    int i;
    for(i = 0 ; i < len ; i++){
        if(isdigit(amount[i]) == 0){
            if(amount[i] == '.'){
                if(periodCount++ > 1){
                    return 0;
                }
            }else{
                return 0;
            }
        }
    }
    return 1;
}

command_state commandchoice(char * command){
    int i;
    for(i = 0; i < strlen(command); i++){
        command[i] = tolower(command[i]);
    }

    if(strcmp(command,"create") == 0){
        return cs_create;
    }else if(strcmp(command,"serve") == 0){
        return cs_serve;
    }else if(strcmp(command,"deposit") == 0){
        return cs_deposit;
    }else if(strcmp(command,"withdraw") == 0){
        return cs_withdraw;
    }else if(strcmp(command,"query") == 0){
        return cs_query;
    }else if(strcmp(command,"end") == 0){
        return cs_end;
    }else if(strcmp(command,"quit") == 0){
        return cs_quit;
    }else{
      return cs_notquit;
    }

    return cs_notquit;
}


void initialize(account ***arr_account){
    *arr_account = (account ** ) malloc(sizeof(account *)*1000);
    int i;
    for(i = 0 ; i < 1000 ; i++){
        (*arr_account)[i] = (account *) malloc(sizeof(account));

        ((*arr_account)[i])->service_flag = -1;

        pthread_mutex_init(&(((*arr_account)[i])->lock), NULL);
    }
    return;
}



int accountsearch(account ** arr_account, char * name){
    int i;
    for(i = 0 ; i < 1000 ; i++){
        if(strcmp( (arr_account[i])->name ,name) == 0){
          return i;
        }
    }
    return -1;
}


void accountcreate(int client_socket_fd, int *numAccount, account ***arr_account, char * name, int * session, pthread_mutex_t * openAuthLock){

    pthread_mutex_lock(openAuthLock);

    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));

    if(strlen(name) <= 0){
      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Name not specified\n") ) < 0 ){
          printf("ERROR: Write: %s\n", strerror(errno));
          pthread_mutex_unlock(openAuthLock);
          exit(1);
      }
      pthread_mutex_unlock(openAuthLock);
      return;
    }

    if(*session >= 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Currently in a session so cannot open account: %s\n",name) ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            pthread_mutex_unlock(openAuthLock);
            exit(1);
        }
        pthread_mutex_unlock(openAuthLock);
        return;
    }

    if(*numAccount >= 1000){
      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Bank cannot hold more than 1000 accounts at this time: %s\n",name) ) < 0 ){
          printf("ERROR: write: %s\n", strerror(errno));
          pthread_mutex_unlock(openAuthLock);
          exit(1);
      }
      pthread_mutex_unlock(openAuthLock);
      return;
    }

    if(accountsearch(*arr_account,name) >= 0 ){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Bank cannot accomodate for same name: %s\n",name) ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            pthread_mutex_unlock(openAuthLock);
            exit(1);
        }
        pthread_mutex_unlock(openAuthLock);
        return;
    }

    account * client_account = (*arr_account)[*numAccount];

    strcpy(client_account->name, name);   
    client_account->balance = 0.0;      
    client_account->service_flag = 0;  

    if ( write(client_socket_fd, obuffer, sprintf(obuffer, "-----Account created: %s -----\n",name) ) < 0 ){
        printf("ERROR: write: %s\n", strerror(errno));
        pthread_mutex_unlock(openAuthLock);
        exit(1);
    }


    *numAccount = *numAccount + 1;

    pthread_mutex_unlock(openAuthLock);
    return;

}

void accountserve(int client_socket_fd,  account ***arr_account, char * name, int *session){
    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));

    if(strlen(name) <= 0){
      if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Name not specified\n") ) < 0 ){
          printf("ERROR: write: %s\n", strerror(errno));
          exit(1);
      }
      return;
    }

    if(*session >= 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Already in session, so cannot open right now: %s\n",name) ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }


    if( (*session = accountsearch(*arr_account,name)) < 0 ){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Account not found: %s\n",name) ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }


    account * client_account = (*arr_account)[*session];



    if( pthread_mutex_trylock(&(client_account->lock)) != 0 ){  
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "-----Currently in use, please try later: %s-----\n",name) ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            exit(1);
        }
        *session = -1;  
          return;
    }


    client_account->service_flag = 1;


    if ( write(client_socket_fd, obuffer, sprintf(obuffer, "-----Sesssion begun: %s -----\n",name) ) < 0 ){
        printf("ERROR: write: %s\n", strerror(errno));
        exit(1);
    }

    return;

}

void accountdeposit(int client_socket_fd, account ***arr_account, char * amount, int *session){
    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));


    if(strlen(amount) <= 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Amount for depositing not specified.\n") ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    if(!isamount(amount)){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Number must be positive.\n") ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }


    if(*session < 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: No session currently exists.\n") ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    double depositvalue = atof(amount);

    account * client_account = (*arr_account)[*session];
    client_account->balance += depositvalue;

    if ( write(client_socket_fd, obuffer, sprintf(obuffer, "-----Your account has deposited that amount.-----\n") ) < 0 ){
        printf("ERROR: write: %s\n", strerror(errno));
        exit(1);
    }

    return;

}

void accountwithdraw(int client_socket_fd, account ***arr_account, char * amount, int *session){
    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));


    if(strlen(amount) <= 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: You must specificy the proper amount for withdrawing.\n") ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    if(!isamount(amount)){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: Number must be positive .\n") ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }


    if(*session < 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: No session exists.\n") ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    double withdrawvalue = atof(amount);

    account * client_account = (*arr_account)[*session];

    if(withdrawvalue > client_account->balance){
      if( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: You can't withdraw more than you have deposited!!.\n") ) < 0 ){
          printf("ERROR: write: %s\n", strerror(errno));
          exit(1);
      }
      return;
    }

    client_account->balance -= withdrawvalue;

    if( write(client_socket_fd, obuffer, sprintf(obuffer, "-----You have withdrawn that amount-----\n") ) < 0 ){
        printf("ERROR: write: %s\n", strerror(errno));
        exit(1);
    }

    return;

}

void accountquery(int client_socket_fd,  account ***arr_account, int *session){
    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));


    if(*session < 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: No session exists.\n") ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }


    double balance = (*arr_account)[*session]->balance;

    if ( write(client_socket_fd, obuffer, sprintf(obuffer, "-----Your balance is: %lf.-----\n",balance) ) < 0 ){
        printf("ERROR: write: %s\n", strerror(errno));
        exit(1);
    }


    return;


}

void accountend(int client_socket_fd, account ***arr_account,  int *session){
    char obuffer[256];
    memset(obuffer, 0, sizeof(obuffer));

    if(*session < 0){
        if ( write(client_socket_fd, obuffer, sprintf(obuffer, "ERROR: No session exists.\n") ) < 0 ){
            printf("ERROR: write: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    account * client_account = (*arr_account)[*session];
    char * name = client_account->name;

    if ( write(client_socket_fd, obuffer, sprintf(obuffer, "-----Session finished: %s-----\n",name) ) < 0 ){
        printf("ERROR: write: %s\n", strerror(errno));
        exit(1);
    }


    client_account->service_flag = 0;

    *session = -1;

    pthread_mutex_unlock(&(client_account->lock));

    return;

}
