#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "process.h"

int main(int argc, char *argv[]) {
    int pipe_data[2];      
    int pipe_ack[2];       
    pid_t pid;
    int file_start_index = 1;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-p pipe_name] file1 [file2 ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if (strcmp(argv[1], "-p") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Error: after -p specify pipe name and files\n");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "Warning: Named pipe not supported\n");
        fprintf(stderr, "Using unnamed pipes\n");
        file_start_index = 3;
    }
    
    if (argc <= file_start_index) {
        fprintf(stderr, "Error: no files specified for copying\n");
        exit(EXIT_FAILURE);
    }
    
    if (pipe(pipe_data) == -1) {
        fprintf(stderr, "Error creating data pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (pipe(pipe_ack) == -1) {
        fprintf(stderr, "Error creating ack pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    pid = fork();
    
    if (pid == -1) {
        fprintf(stderr, "Fork error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        close(pipe_data[1]);
        close(pipe_ack[0]);
        
        child_process(pipe_data[0], pipe_ack[1]);
        
        close(pipe_data[0]);
        close(pipe_ack[1]);
        exit(EXIT_SUCCESS);
    } else {
        close(pipe_data[0]);
        close(pipe_ack[1]);
        
        parent_process(pipe_data[1], pipe_ack[0], 
                      argv + file_start_index, 
                      argc - file_start_index);
        
        close(pipe_data[1]);
        close(pipe_ack[0]);
        
        int status;
        wait(&status);
        printf("Parent: child process finished with code %d\n", 
               WEXITSTATUS(status));
    }
    
    return 0;
}
