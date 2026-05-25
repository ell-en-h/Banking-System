#include <pthread.h>
#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "bank.h"

void print_help() {
    std::cout << "\n========== Доступные команды ==========\n";
    std::cout << "get balance <A>        - показать текущий баланс счета A\n";
    std::cout << "get min <A>            - показать минимальный лимит счета A\n";
    std::cout << "get max <A>            - показать максимальный лимит счета A\n";
    std::cout << "get frozen <A>         - показать, заморожен ли счет A\n";
    std::cout << "freeze <A>             - заморозить счет A\n";
    std::cout << "unfreeze <A>           - разморозить счет A\n";
    std::cout << "transfer <A> <B> <X>   - перевести X со счета A на счет B\n";
    std::cout << "add_all <X>            - зачислить X на все счета\n";
    std::cout << "sub_all <X>            - списать X со всех счетов\n";
    std::cout << "set min <A> <X>        - установить минимальный лимит X для счета A\n";
    std::cout << "set max <A> <X>        - установить максимальный лимит X для счета A\n";
    std::cout << "help                   - показать это сообщение\n";
    std::cout << "exit                   - выйти\n";
    std::cout << "=======================================\n\n";
}

bool valid_account(Bank* bank, int acc) {
    return acc >= 0 && acc < bank->num_accounts;
}

int main() {
    std::cout << "Клиент банка запущен...\n";

    int fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd == -1) {
        perror("Не удалось подключиться к банку");
        std::cout << "Сначала нужно запустить инициализатор ./init N\n";
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat не удался");
        close(fd);
        return 1;
    }

    void* ptr = mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap не удался");
        close(fd);
        return 1;
    }

    close(fd);

    Bank* bank = static_cast<Bank*>(ptr);
    int n = bank->num_accounts;

    std::cout << "Счетов в банке: " << n << "\n";
    print_help();

    std::string line;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "exit") break;
        if (cmd == "help") { print_help(); continue; }

        if (cmd == "get") {
            std::string type;
            int acc_id;

            if (!(iss >> type >> acc_id)) {
                std::cout << "Ошибка: неверный формат команды get\n";
                continue;
            }
            if (!valid_account(bank, acc_id)) {
                std::cout << "Ошибка: неверный номер счета\n";
                continue;
            }

            Account& acc = bank->accounts[acc_id];
            pthread_mutex_lock(&acc.lock);

            if (type == "balance") {
                std::cout << "Баланс счета " << acc_id << ": " << acc.balance << " драм\n";
            } else if (type == "min") {
                std::cout << "Мин. лимит счета " << acc_id << ": " << acc.min_balance << " драм\n";
            } else if (type == "max") {
                std::cout << "Макс. лимит счета " << acc_id << ": " << acc.max_balance << " драм\n";
            } else if (type == "frozen") {
                std::cout << "Счет " << acc_id << " " << (acc.is_frozen ? "заморожен" : "не заморожен") << "\n";
            } else {
                std::cout << "Ошибка: неизвестный параметр для get\n";
            }

            pthread_mutex_unlock(&acc.lock);
        }

        else if (cmd == "freeze") {
            int acc_id;
            if (!(iss >> acc_id)) { std::cout << "Ошибка: неверный формат команды freeze\n"; continue; }
            if (!valid_account(bank, acc_id)) { std::cout << "Ошибка: неверный номер счета\n"; continue; }

            Account& acc = bank->accounts[acc_id];
            pthread_mutex_lock(&acc.lock);

            if (acc.is_frozen) {
                std::cout << "Ошибка: счет уже заморожен\n";
            } else {
                acc.is_frozen = true;
                std::cout << "Успех: счет " << acc_id << " заморожен\n";
            }

            pthread_mutex_unlock(&acc.lock);
        }

        else if (cmd == "unfreeze") {
            int acc_id;
            if (!(iss >> acc_id)) { std::cout << "Ошибка: неверный формат команды unfreeze\n"; continue; }
            if (!valid_account(bank, acc_id)) { std::cout << "Ошибка: неверный номер счета\n"; continue; }

            Account& acc = bank->accounts[acc_id];
            pthread_mutex_lock(&acc.lock);

            if (!acc.is_frozen) {
                std::cout << "Ошибка: счет и так не заморожен\n";
            } else {
                acc.is_frozen = false;
                std::cout << "Успех: счет " << acc_id << " разморожен\n";
            }

            pthread_mutex_unlock(&acc.lock);
        }

        else if (cmd == "transfer") {
            int from, to;
            long long amount;

            if (!(iss >> from >> to >> amount)) {
                std::cout << "Ошибка: неверный формат команды transfer\n";
                continue;
            }
            if (!valid_account(bank, from) || !valid_account(bank, to)) {
                std::cout << "Ошибка: неверный номер счета\n";
                continue;
            }
            if (amount <= 0) {
                std::cout << "Ошибка: сумма перевода должна быть положительной\n";
                continue;
            }
            if (from == to) {
                std::cout << "Ошибка: нельзя переводить на тот же самый счет\n";
                continue;
            }

            int first_id = (from < to) ? from : to;
            int second_id = (from < to) ? to : from;

            pthread_mutex_lock(&bank->accounts[first_id].lock);
            pthread_mutex_lock(&bank->accounts[second_id].lock);

            Account& acc_from = bank->accounts[from];
            Account& acc_to = bank->accounts[to];

            if (acc_from.is_frozen || acc_to.is_frozen) {
                std::cout << "Ошибка: один из счетов заморожен\n";
            } else if (acc_from.balance - amount < acc_from.min_balance) {
                std::cout << "Ошибка: перевод нарушает минимальный лимит счета " << from << "\n";
            } else if (acc_to.balance + amount > acc_to.max_balance) {
                std::cout << "Ошибка: перевод нарушает максимальный лимит счета " << to << "\n";
            } else {
                acc_from.balance -= amount;
                acc_to.balance += amount;
                std::cout << "Успех: переведено " << amount << " со счета " << from << " на счет " << to << "\n";
            }

            pthread_mutex_unlock(&bank->accounts[second_id].lock);
            pthread_mutex_unlock(&bank->accounts[first_id].lock);
        }

        else if (cmd == "add_all") {
            long long amount;
            if (!(iss >> amount)) { std::cout << "Ошибка: неверный формат команды add_all\n"; continue; }
            if (amount <= 0) { std::cout << "Ошибка: сумма должна быть положительной\n"; continue; }

	    for (int i = 0; i < n; ++i) pthread_mutex_lock(&bank->accounts[i].lock);

            bool ok = true;
            for (int i = 0; i < n; ++i) {
                if (bank->accounts[i].is_frozen) {
                    std::cout << "Ошибка: счет " << i << " заморожен\n";
                    ok = false; break;
                }
                if (bank->accounts[i].balance + amount > bank->accounts[i].max_balance) {
                    std::cout << "Ошибка: операция нарушает максимальный лимит счета " << i << "\n";
                    ok = false; break;
                }
            }

            if (ok) {
                for (int i = 0; i < n; ++i) bank->accounts[i].balance += amount;
                std::cout << "Успех: на все счета зачислено по " << amount << "\n";
            }

            for (int i = n - 1; i >= 0; --i) pthread_mutex_unlock(&bank->accounts[i].lock);
        }

        else if (cmd == "sub_all") {
            long long amount;
            if (!(iss >> amount)) { std::cout << "Ошибка: неверный формат команды sub_all\n"; continue; }
            if (amount <= 0) { std::cout << "Ошибка: сумма должна быть положительной\n"; continue; }

            for (int i = 0; i < n; ++i) pthread_mutex_lock(&bank->accounts[i].lock);

            bool ok = true;
            for (int i = 0; i < n; ++i) {
                if (bank->accounts[i].is_frozen) {
                    std::cout << "Ошибка: счет " << i << " заморожен\n";
                    ok = false; break;
                }
                if (bank->accounts[i].balance - amount < bank->accounts[i].min_balance) {
                    std::cout << "Ошибка: операция нарушает минимальный лимит счета " << i << "\n";
                    ok = false; break;
                }
            }

            if (ok) {
                for (int i = 0; i < n; ++i) bank->accounts[i].balance -= amount;
                std::cout << "Успех: со всех счетов списано по " << amount << "\n";
            }

            for (int i = n - 1; i >= 0; --i) pthread_mutex_unlock(&bank->accounts[i].lock);
        }

        else if (cmd == "set") {
            std::string type;
            int acc_id;
            long long value;

            if (!(iss >> type >> acc_id >> value)) {
                std::cout << "Ошибка: неверный формат команды set\n";
                continue;
            }
            if (!valid_account(bank, acc_id)) {
                std::cout << "Ошибка: неверный номер счета\n";
                continue;
            }

            Account& account = bank->accounts[acc_id];
            pthread_mutex_lock(&account.lock);

            if (type == "min") {
                if (value > account.max_balance) {
                    std::cout << "Ошибка: min не может быть больше max\n";
                } else if (value > account.balance) {
                    std::cout << "Ошибка: текущий баланс меньше нового минимального лимита\n";
                } else {
                    account.min_balance = value;
                    std::cout << "Успех: для счета " << acc_id << " установлен min = " << value << "\n";
                }
            } else if (type == "max") {
                if (value < account.min_balance) {
                    std::cout << "Ошибка: max не может быть меньше min\n";
                } else if (value < account.balance) {
                    std::cout << "Ошибка: текущий баланс больше нового максимального лимита\n";
                } else {
                    account.max_balance = value;
                    std::cout << "Успех: для счета " << acc_id << " установлен max = " << value << "\n";
                }
            } else {
                std::cout << "Ошибка: можно использовать только set min или set max\n";
            }

            pthread_mutex_unlock(&account.lock);
        }

        else {
            std::cout << "Ошибка: неизвестная команда\n";
            print_help();
        }
    }

    munmap(ptr, st.st_size);
    return 0;
}
