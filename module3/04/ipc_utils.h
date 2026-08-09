#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

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


union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};


extern int shmid;           
extern int semid;           
extern SharedMemory* shm;   

int init_shared_memory(key_t key, size_t size);

void detach_shared_memory();

void cleanup_shared_memory();

int init_semaphore(key_t key);

void semaphore_lock(int semid);

void semaphore_unlock(int semid);

void cleanup_semaphore();

void cleanup_all_ipc();

#endif
