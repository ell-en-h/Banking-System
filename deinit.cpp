#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include "bank.h"

int main() {
    int fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    size_t size = st.st_size;

    Bank *bank = (Bank*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bank == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    close(fd);

    for (int i = 0; i < bank->num_accounts; i++) {
        pthread_mutex_destroy(&bank->accounts[i].lock);
    }

    if (munmap(bank, size) == -1) {
        perror("munmap");
        return 1;
    }

    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink");
        return 1;
    }

    std::cout << "Банк успешно деинициализирован. Память очищена." << std::endl;

    return 0;
}
