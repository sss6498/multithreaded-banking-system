#include <pthread.h>
#include <ctype.h>

    typedef enum command_state_ {
        cs_create,
        cs_serve,
        cs_deposit,
        cs_withdraw,
        cs_query,
        cs_end,
        cs_quit,
        cs_notquit,
    } command_state;

    typedef struct account_ {
        char name[256];
        double balance;
        int service_flag;              // -1: Not created, 1: In service,  0: Not in service
        pthread_mutex_t lock;
    } account;

    int isamount(char * amount);

    command_state commandchoice(char * command);

    void initialize(account ***arr_account);

    // searches bank for account.
    // return < 0 if not found. And index number if found.
    int accountsearch(account ** arr_account, char * name);

    void accountcreate(int client_socket_fd, int *numAccount, account ***arr_account, char * name, int *session, pthread_mutex_t * openAuthLock);

    void accountserve(int client_socket_fd,  account ***arr_account, char * name, int *session);

    void accountdeposit(int client_socket_fd, account ***arr_account, char * amount, int *session);

    void accountwithdraw(int client_socket_fd, account ***arr_account, char * amount, int *session);

    void accountquery(int client_socket_fd,  account ***arr_account, int *session);

    void accountend(int client_socket_fd, account ***arr_account,  int *session);

 
