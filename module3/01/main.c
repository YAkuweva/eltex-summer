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
    int use_named_pipe = 0;
    char *pipe_name = NULL;
    char pipe_data_name[256];
    char pipe_ack_name[256];
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-p pipe_name] file1 [file2 ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if (strcmp(argv[1], "-p") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Error: after -p specify pipe name and files\n");
            exit(EXIT_FAILURE);
        }
        pipe_name = argv[2];
        use_named_pipe = 1;
        file_start_index = 3;
        
        snprintf(pipe_data_name, sizeof(pipe_data_name), "%s_data", pipe_name);
        snprintf(pipe_ack_name, sizeof(pipe_ack_name), "%s_ack", pipe_name);
        
        if (mkfifo(pipe_data_name, 0666) == -1 && errno != EEXIST) {
            fprintf(stderr, "Error creating named pipe '%s': %s\n", 
                    pipe_data_name, strerror(errno));
            exit(EXIT_FAILURE);
        }
        printf("Created named pipe: %s\n", pipe_data_name);
        
        if (mkfifo(pipe_ack_name, 0666) == -1 && errno != EEXIST) {
            fprintf(stderr, "Error creating named pipe '%s': %s\n", 
                    pipe_ack_name, strerror(errno));
            unlink(pipe_data_name);
            exit(EXIT_FAILURE);
        }
        printf("Created named pipe: %s\n", pipe_ack_name);
    }
    
    if (argc <= file_start_index) {
        fprintf(stderr, "Error: no files specified for copying\n");
        exit(EXIT_FAILURE);
    }
    
    if (use_named_pipe) {
        pid = fork();
        
        if (pid == -1) {
            fprintf(stderr, "Fork error: %s\n", strerror(errno));
            unlink(pipe_data_name);
            unlink(pipe_ack_name);
            exit(EXIT_FAILURE);
        }
        
        if (pid == 0) {
            int data_fd = open(pipe_data_name, O_RDONLY);
            if (data_fd == -1) {
                fprintf(stderr, "Child: error opening data pipe: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            
            int ack_fd = open(pipe_ack_name, O_WRONLY);
            if (ack_fd == -1) {
                fprintf(stderr, "Child: error opening ack pipe: %s\n", strerror(errno));
                close(data_fd);
                exit(EXIT_FAILURE);
            }
            
            child_process(data_fd, ack_fd);
            
            close(data_fd);
            close(ack_fd);
            exit(EXIT_SUCCESS);
        } else {
            int data_fd = open(pipe_data_name, O_WRONLY);
            if (data_fd == -1) {
                fprintf(stderr, "Parent: error opening data pipe: %s\n", strerror(errno));
                wait(NULL);
                unlink(pipe_data_name);
                unlink(pipe_ack_name);
                exit(EXIT_FAILURE);
            }
            
            int ack_fd = open(pipe_ack_name, O_RDONLY);
            if (ack_fd == -1) {
                fprintf(stderr, "Parent: error opening ack pipe: %s\n", strerror(errno));
                close(data_fd);
                wait(NULL);
                unlink(pipe_data_name);
                unlink(pipe_ack_name);
                exit(EXIT_FAILURE);
            }
            
            parent_process(data_fd, ack_fd, 
                          argv + file_start_index, 
                          argc - file_start_index);
            
            close(data_fd);
            close(ack_fd);
            
            int status;
            wait(&status);
            printf("Parent: child process finished with code %d\n", 
                   WEXITSTATUS(status));
            
            unlink(pipe_data_name);
            unlink(pipe_ack_name);
            printf("Named pipes removed\n");
        }
    } else {
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
    }
    
    return 0;
}
