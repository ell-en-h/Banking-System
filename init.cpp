#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include "bank.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <N>" << std::endl;
        return 1;
    }

    int n = std::stoi(argv[1]);
    size_t size = sizeof(Bank) + n * sizeof(Account);

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); return 1; }

    ftruncate(fd, size);

    Bank *bank = (Bank*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bank == MAP_FAILED) { perror("mmap"); return 1; }

    bank->num_accounts = n;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    for (int i = 0; i < n; i++) {
        bank->accounts[i].balance = 0;
        bank->accounts[i].min_balance = 0;
        bank->accounts[i].max_balance = 10000;
        bank->accounts[i].is_frozen = false;
        pthread_mutex_init(&bank->accounts[i].lock, &attr);
    }

    std::cout << "Банк на " << n << " счетов создан " << std::endl;

    pthread_mutexattr_destroy(&attr);
    munmap(bank, size);
    close(fd);

    return 0;
}
