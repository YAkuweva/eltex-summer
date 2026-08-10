#include "ipc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int shmid = -1;
int semid = -1;
SharedMemory* shm = NULL;


int init_shared_memory(key_t key, size_t size) {
    shmid = shmget(key, size, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        return -1;
    }
    
    shm = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        shmid = -1;
        return -1;
    }
    
    memset(shm, 0, sizeof(SharedMemory));
    shm->memory_start = (char*)shm + sizeof(SharedMemory);
    shm->memory_end = (char*)shm + size;
    shm->total_size = size;
    shm->producer_finished = false;
    
    return 0;
}

void detach_shared_memory() {
    if (shm != NULL) {
        shmdt(shm);
        shm = NULL;
    }
}

void cleanup_shared_memory() {
    detach_shared_memory();
    if (shmid > 0) {
        shmctl(shmid, IPC_RMID, NULL);
        shmid = -1;
    }
}


int init_semaphore(key_t key) {
    semid = semget(key, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        return -1;
    }
    
    union semun arg;
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        return -1;
    }
    
    return 0;
}

void semaphore_lock(int semid) {
    struct sembuf op = {0, -1, 0};  
    if (semop(semid, &op, 1) == -1) {
        perror("semop lock");
        exit(1);
    }
}

void semaphore_unlock(int semid) {
    struct sembuf op = {0, 1, 0};   
    if (semop(semid, &op, 1) == -1) {
        perror("semop unlock");
        exit(1);
    }
}

void cleanup_semaphore() {
    if (semid > 0) {
        semctl(semid, 0, IPC_RMID);
        semid = -1;
    }
}

void cleanup_all_ipc() {
    cleanup_shared_memory();
    cleanup_semaphore();
}
