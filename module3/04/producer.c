#include "ipc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

void producer_function() {
    printf("Producer started (PID: %d)\n", getpid());
    srand(time(NULL) ^ (getpid() << 16));
    
    size_t used_memory = sizeof(SharedMemory);
    int block_count = 0;
    int blocks_since_check = 0;
    const int BATCH_SIZE = 5;      
    while (1) {
        size_t remaining = shm->total_size - used_memory;
        
        if (remaining < sizeof(int) + sizeof(Node*) + sizeof(int)) {
            semaphore_lock(semid);
            shm->producer_finished = true;
            semaphore_unlock(semid);
            printf("Producer: no more memory, finished. Created %d blocks\n", block_count);
            break;
        }
        
        int count = (rand() % 50) + 20;
        
        if (remaining < sizeof(int) + sizeof(Node*) + count * sizeof(int)) {
            semaphore_lock(semid);
            shm->producer_finished = true;
            semaphore_unlock(semid);
            printf("Producer: not enough memory for next array, finished. Created %d blocks\n", block_count);
            break;
        }
        
        Node* new_node = (Node*)((char*)shm->memory_start + used_memory);
        new_node->count = count;
        new_node->data = (int*)((char*)new_node + sizeof(Node));
        new_node->next = NULL;
        
        for (int i = 0; i < count; i++) {
            new_node->data[i] = (rand() % 100) + 1;
        }
        
        semaphore_lock(semid);
        
        if (shm->head == NULL) {
            shm->head = new_node;
            shm->tail = new_node;
        } else {
            shm->tail->next = new_node;
            shm->tail = new_node;
        }
        
        used_memory += sizeof(Node) + count * sizeof(int);
        
        semaphore_unlock(semid);
        
        block_count++;
        blocks_since_check++;
        printf("Producer: created block #%d with %d numbers\n", block_count, count);
        
        if (blocks_since_check >= BATCH_SIZE) {
            semaphore_lock(semid);
            
            bool all_processed = true;
            Node* current = shm->head;
            while (current != NULL) {
                if (current->count > 0) {  
                    all_processed = false;
                    break;
                }
                current = current->next;
            }
            semaphore_unlock(semid);
            
            if (all_processed && block_count > 0) {
                semaphore_lock(semid);
                shm->producer_finished = true;
                semaphore_unlock(semid);
                printf("Producer: all blocks are processed, finishing. Created %d blocks\n", block_count);
                break;
            }
            
            blocks_since_check = 0;
            
            if (!all_processed) {
                printf("Producer: waiting for consumers to process blocks...\n");
                sleep(1);
            }
        }
        
        usleep((rand() % 100 + 50) * 1000);
    }
    
    printf("Producer: exiting\n");
    exit(0);
}
