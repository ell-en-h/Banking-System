#ifndef BANK_H
#define BANK_H
#include <pthread.h>

#define SHM_NAME "/transparent_bank_shm"

struct Account {
    long long balance;
    long long min_balance;
    long long max_balance;
    bool is_frozen;
    pthread_mutex_t lock;
};

struct Bank {
    int num_accounts;
    Account accounts[]; 
};

#endif
