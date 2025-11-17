#include <stdio.h>
#include <thread>
#include <vector>
#include <semaphore.h>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

// Globals
sem_t tellerReady, customerReady, safeAccess, managerAccess, doorAccess;
std::mutex queueMutex;
std::mutex outputMutex;
std::condition_variable queueCV;
std::vector<int> queue;
std::unordered_map<int, sem_t> customerDone;
std::atomic<int> numCustomersServed(0);
std::atomic<int> numTellersDone(0);
bool bankOpen = false;
bool bankClosing = false;

void teller(int id);
void customer(int id);

int main() {
    // 1. Initialize semaphores
    sem_init(&tellerReady, 0, 0);
    sem_init(&customerReady, 0, 0);
    sem_init(&safeAccess, 0, 2);
    sem_init(&managerAccess, 0, 1);
    sem_init(&doorAccess, 0, 2);

    std::vector<std::thread> tellerThreads;
    std::vector<std::thread> customerThreads;

    // 2. Launch tellers
    for (int i = 0; i < 3; ++i) {
        tellerThreads.emplace_back(teller, i + 1);
    }

    // 3. Open bank
    bankOpen = true;
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        printf("Bank is now open.\n");
    }

    // 4. Launch 50 customers
    for (int i = 0; i < 50; ++i) {
        int customerId = i + 1;
        sem_t doneSemaphore;
        sem_init(&doneSemaphore, 0, 0);
        customerDone[customerId] = doneSemaphore;
        customerThreads.emplace_back(customer, customerId);
    }

    // 5. Wait for all customers
    for (auto &th : customerThreads) {
        th.join();
    }

    // 6. Start shutdown: tellers finish when no customers left
    bankClosing = true;
    for (int i = 0; i < 3; ++i) {
        sem_post(&tellerReady);   // wake tellers so they can exit
    }

    // 7. Wait for all tellers to mark themselves done
    while (numTellersDone < 3) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    {
        std::lock_guard<std::mutex> lock(outputMutex);
        printf("Bank is now closed.\n");
    }

    // 8. Join teller threads
    for (auto &th : tellerThreads) {
        th.join();
    }

    // 9. Cleanup semaphores
    for (auto &p : customerDone) {
        sem_destroy(&p.second);
    }
    sem_destroy(&tellerReady);
    sem_destroy(&customerReady);
    sem_destroy(&safeAccess);
    sem_destroy(&managerAccess);
    sem_destroy(&doorAccess);

    return 0;
}



void teller(int id) {}

void customer(int id) {}

