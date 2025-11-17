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

    // Launch teller threads
    for (int i = 0; i < 3; ++i) {
        tellerThreads.emplace_back(teller, i + 1);
    }

    // Open the bank after tellers are ready
    bankOpen = true;
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Bank is now open.\n";
    }

    // Launch customer threads
    for (int i = 0; i < 50; ++i) {
        int customerId = i + 1;
        sem_t doneSemaphore;
        sem_init(&doneSemaphore, 0, 0);
        customerDone[customerId] = doneSemaphore;  // Initialize completion semaphore for each customer
        customerThreads.emplace_back(customer, customerId);
    }

    // Join customer threads
    for (auto& th : customerThreads) {
        th.join();
    }

    // Signal to close the bank once all customers are served
    bankClosing = true;
    for (int i = 0; i < 3; ++i) {
        sem_post(&tellerReady); // Wake up each teller so they can check bankClosing
    }

    // Wait until all tellers have finished processing
    while (numTellersDone < 3) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Bank is now closed.\n";
    }

    // Join teller threads
    for (auto& th : tellerThreads) {
        th.join();
    }

    // Destroy semaphores
    for (auto& pair : customerDone) {
        sem_destroy(&pair.second);
    }
    sem_destroy(&tellerReady);
    sem_destroy(&customerReady);
    sem_destroy(&safeAccess);
    sem_destroy(&managerAccess);
    sem_destroy(&doorAccess);

    return 0;
}

void teller(int id) {
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Teller " << id << " is ready to serve.\n";
    }

    while (true) {
        // Check if bank is closing and all customers have been served
        if (bankClosing && numCustomersServed >= 50 && queue.empty()) {
            break;  // Exit loop if bank is closing, no more customers remain
        }
       
        sem_wait(&tellerReady);  // Wait for customer

        // Check again if the bank is closing in case we were signaled for closure
        if (bankClosing && numCustomersServed >= 50 && queue.empty()) {
            break;
        }

        int customerId;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (queue.empty()) {
                queueCV.wait(lock, [] { return !queue.empty(); });
            }
            customerId = queue.front();
            queue.erase(queue.begin());
        }
        sem_post(&customerReady);  // Signal customer to proceed

        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Teller " << id << " is serving Customer " << customerId << ".\n";
        }

        // Interact with manager if withdrawal
        bool isWithdrawal = rand() % 2;
        if (isWithdrawal) {
            sem_wait(&managerAccess);
            {
                std::lock_guard<std::mutex> lock(outputMutex);
                std::cout << "Teller " << id << " getting manager's permission.\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5 + rand() % 26));
            sem_post(&managerAccess);
        }

        // Enter the safe to complete transaction
        sem_wait(&safeAccess);
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Teller " << id << " in safe performing transaction for Customer " << customerId << ".\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10 + rand() % 41));
        sem_post(&safeAccess);

        // Transaction done
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Teller " << id << " completed transaction for Customer " << customerId << ".\n";
        }
        ++numCustomersServed;

        // Signal customer that transaction is done
        sem_post(&customerDone[customerId]);
    }

    ++numTellersDone; // Increment the number of finished tellers
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Teller " << id << " is leaving the bank.\n";
    }
}

void customer(int id) {
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Customer " << id << " going to the bank.\n";
    }

    sem_wait(&doorAccess);  // Wait for access through door
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.push_back(id);
    }
    sem_post(&tellerReady);  // Notify tellers a customer is ready
    sem_post(&doorAccess);   // Release door access

    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Customer " << id << " is in line.\n";
    }
    sem_wait(&customerReady);  // Wait for teller to be ready

    // Transaction details
    bool isWithdrawal = rand() % 2;
    std::string transactionType = isWithdrawal ? "withdrawal" : "deposit";
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Customer " << id << " requests a " << transactionType << " transaction.\n";
    }

    // Wait for teller to complete transaction
    sem_wait(&customerDone[id]);  // Wait for teller to signal transaction completion
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "Customer " << id << " has completed transaction and is leaving.\n";
    }
}
