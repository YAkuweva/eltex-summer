#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>


typedef struct Node {
    int count;              
    int* data;              
    struct Node* next;      
} Node;

typedef struct {
    Node* head;             
    Node* tail;             
    void* memory_start;     
    void* memory_end;       
    size_t total_size;      
    bool producer_finished; 
} SharedMemory;


extern int shm_fd;          
extern sem_t* sem;          
extern SharedMemory* shm;   
extern char* shm_name;      

int init_shared_memory(const char* name, size_t size);

void detach_shared_memory();

void cleanup_shared_memory();

int init_semaphore(const char* name, unsigned int initial_value);

void semaphore_lock(sem_t* sem);

void semaphore_unlock(sem_t* sem);

void cleanup_semaphore();

void cleanup_all_ipc();

#endif
