#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include "bank.h"

void print_menu() {
    std::cout << "\n=== МЕНЮ БАНКА ===\n"
              << "1. Проверить баланс\n"
              << "2. Пополнить счет\n"
              << "3. Снять деньги\n"
              << "4. Выход\n"
              << "Выберите действие: ";
}

int main() {
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        std::cerr << "Ошибка: Банк не запущен\n";
        return 1;
    }

    Bank *temp_bank = (Bank*)mmap(NULL, sizeof(Bank), PROT_READ, MAP_SHARED, fd, 0);
    if (temp_bank == MAP_FAILED) { perror("mmap temp"); return 1; }
    
    int n = temp_bank->num_accounts;
    munmap(temp_bank, sizeof(Bank));

    size_t full_size = sizeof(Bank) + n * sizeof(Account);
    Bank *bank = (Bank*)mmap(NULL, full_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bank == MAP_FAILED) { perror("mmap full"); return 1; }
    close(fd);

    std::cout << "Успешно подключено к банку Всего счетов в системе: " << n << std::endl;

    int choice;
    int acc_id;
    long long amount;

    while (true) {
        print_menu();
        if (!(std::cin >> choice)) break;
        if (choice == 4) break;

        std::cout << "Введите номер счета (0 - " << (n - 1) << "): ";
        std::cin >> acc_id;

        if (acc_id < 0 || acc_id >= n) {
            std::cout << "Ошибка: Неверный номер счета\n";
            continue;
        }

        Account &acc = bank->accounts[acc_id];

        if (choice == 1) {
            pthread_mutex_lock(&acc.lock);
            std::cout << "Баланс счета #" << acc_id << ": " << acc.balance << " драм\n";
            pthread_mutex_unlock(&acc.lock);
        } 
        else if (choice == 2) {
            std::cout << "Введите сумму для пополнения: ";
            std::cin >> amount;
            if (amount <= 0) { std::cout << "Сумма должна быть положительной\n"; continue; }

            pthread_mutex_lock(&acc.lock);
            if (acc.balance + amount > acc.max_balance) {
                std::cout << "Ошибка: Превышен максимальный лимит счета (" << acc.max_balance << ")\n";
            } else {
                acc.balance += amount;
                std::cout << "Новый баланс: " << acc.balance << " драм\n";
            }
            pthread_mutex_unlock(&acc.lock);
        } 
        else if (choice == 3) {
            std::cout << "Введите сумму для снятия: ";
            std::cin >> amount;
            if (amount <= 0) { std::cout << "Сумма должна быть положительной\n"; continue; }

            pthread_mutex_lock(&acc.lock);
            if (acc.balance - amount < acc.min_balance) {
                std::cout << "Ошибка: Недостаточно средств Минимальный баланс: " << acc.min_balance << "\n";
            } else {
                acc.balance -= amount;
                std::cout << "Новый баланс: " << acc.balance << " драм\n";
            }
            pthread_mutex_unlock(&acc.lock);
        }
    }

    std::cout << "Отключение от банка...\n";
    munmap(bank, full_size);
    return 0;
}
