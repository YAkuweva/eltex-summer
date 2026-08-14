#include "ipc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int shm_fd = -1;
sem_t* sem = NULL;
SharedMemory* shm = NULL;
char* shm_name = NULL;
char* sem_name = NULL;

int init_shared_memory(const char* name, size_t size) {
     shm_name = strdup(name);
    if (shm_name == NULL) {
        perror("strdup");
        return -1;
    }

    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        free(shm_name);
        shm_name = NULL;
        return -1;
    }
    
    if (ftruncate(shm_fd, size) == -1) {
        perror("ftruncate");
        close(shm_fd);
        shm_fd = -1;
        shm_unlink(name);
        free(shm_name);
        shm_name = NULL;
        return -1;
    }
    
    shm = (SharedMemory*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        shm_fd = -1;
        shm_unlink(name);
        free(shm_name);
        shm_name = NULL;
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
    if (shm != NULL && shm != MAP_FAILED) {
        munmap(shm, sizeof(SharedMemory));
        shm = NULL;
    }
}

void cleanup_shared_memory() {
    detach_shared_memory();
    
    if (shm_fd > 0) {
        close(shm_fd);
        shm_fd = -1;
    }
    
    if (shm_name != NULL) {
        shm_unlink(shm_name);
        free(shm_name);
        shm_name = NULL;
    }
}

int init_semaphore(const char* name, unsigned int initial_value) {
    sem_name = strdup(name);
    if (sem_name == NULL) {
        perror("strdup");
        return -1;
    }
    
    sem = sem_open(name, O_CREAT, 0666, initial_value);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        free(sem_name);
        sem_name = NULL;
        return -1;
    }
    
    return 0;
}

void semaphore_lock(sem_t* sem) {

    if (sem_wait(sem) == -1) {
        perror("sem_wait");
        exit(1);
    }
}

void semaphore_unlock(sem_t* sem) {
    if (sem_post(sem) == -1) {
        perror("sem_post");
        exit(1);
    }
}

void cleanup_semaphore() {
    if (sem != NULL && sem != SEM_FAILED) {
        sem_close(sem);
        sem = NULL;
    }
    
    if (sem_name != NULL) {
        sem_unlink(sem_name);
        free(sem_name);
        sem_name = NULL;
    }
}

void cleanup_all_ipc() {
    cleanup_shared_memory();
    cleanup_semaphore();
}
