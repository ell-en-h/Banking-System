#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include "bank.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <N>" << std::endl;
        return 1;
    }

    int n = std::stoi(argv[1]);
    if (n <= 0) {
        std::cerr << "Ошибка: Количество счетов должно быть больше 0" << std::endl;
        return 1;
    }

    size_t size = sizeof(Bank) + n * sizeof(Account);

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) { 
        perror("shm_open"); 
        return 1; 
    }

    if (ftruncate(fd, size) == -1) {
        perror("ftruncate");
        close(fd);
        return 1;
    }

    Bank *bank = (Bank*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bank == MAP_FAILED) { 
        perror("mmap"); 
        close(fd);
        return 1; 
    }

    close(fd);

    bank->num_accounts = n;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    for (int i = 0; i < n; i++) {
        bank->accounts[i].balance = 0;
        bank->accounts[i].min_balance = 0;
        bank->accounts[i].max_balance = 1000000;
        bank->accounts[i].is_frozen = false;
        pthread_mutex_init(&bank->accounts[i].lock, &attr);
    }

    pthread_mutexattr_destroy(&attr);
    munmap(bank, size);

    std::cout << "Банк на " << n << " счетов создан " << std::endl;

    return 0;
}
