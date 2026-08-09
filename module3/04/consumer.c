#include "ipc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

void consumer_function(int id) {
    printf("Consumer %d started (PID: %d)\n", id, getpid());
    srand(time(NULL) ^ (getpid() << 16) ^ (id << 8));
    
    int processed_blocks = 0;
    int wait_count = 0;
    
    while (1) {
        bool producer_done = false;
        bool has_unprocessed = false;
        
        semaphore_lock(semid);
        producer_done = shm->producer_finished;
        
        Node* current = shm->head;
        while (current != NULL) {
            if (current->count > 0) {
                has_unprocessed = true;
                break;
            }
            current = current->next;
        }
        semaphore_unlock(semid);
        
        if (producer_done && !has_unprocessed) {
            printf("Consumer %d: all blocks processed, exiting\n", id);
            break;
        }
        
        if (!has_unprocessed && !producer_done) {
            wait_count++;
            if (wait_count % 5 == 0) {
                printf("Consumer %d: waiting for new blocks... (%d waits)\n", id, wait_count);
            }
            usleep((rand() % 300 + 100) * 1000);
            continue;
        }
        
        wait_count = 0;
        
        semaphore_lock(semid);
        
        current = shm->head;
        while (current != NULL) {
            if (current->count > 0) {
                break;
            }
            current = current->next;
        }
        
        if (current == NULL) {
            semaphore_unlock(semid);
            usleep((rand() % 300 + 100) * 1000);
            continue;
        }
        
        int count = current->count;
        int* data = current->data;
        current->count = 0;  
        
        semaphore_unlock(semid);
        
        int min_val = data[0];
        int max_val = data[0];
        for (int i = 1; i < count; i++) {
            if (data[i] < min_val) min_val = data[i];
            if (data[i] > max_val) max_val = data[i];
        }
        
        processed_blocks++;
        printf("Consumer %d: processed block with %d numbers, min=%d, max=%d\n", 
               id, count, min_val, max_val);
        
        usleep((rand() % 300 + 100) * 1000);
    }
    
    printf("Consumer %d: processed total %d blocks\n", id, processed_blocks);
    exit(0);
}
