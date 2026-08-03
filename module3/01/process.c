#include "process.h"
#include "file_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define MAX_FILENAME 256

void parent_process(int data_write_fd, int ack_read_fd, char **filenames, int file_count) {
    char buffer[BUFFER_SIZE];
    char copy_name[MAX_FILENAME];
    int bytes_read;
    int file_fd;
    char header[512];
    off_t file_size;
    char ack[10];
    
    read(ack_read_fd, ack, sizeof(ack));
    printf("Parent: received ready message - %s\n", ack);
    
    for (int i = 0; i < file_count; i++) {
        if (!file_exists(filenames[i])) {
            fprintf(stderr, "Error: file '%s' does not exist\n", filenames[i]);
            continue;
        }
        
        file_fd = open(filenames[i], O_RDONLY);
        if (file_fd == -1) {
            fprintf(stderr, "Error: cannot open file '%s' - %s\n", 
                    filenames[i], strerror(errno));
            continue;
        }
        
        file_size = get_file_size(filenames[i]);
        create_copy_name(filenames[i], copy_name);
        
        snprintf(header, sizeof(header), "FILE:%s|SIZE:%ld", 
                 filenames[i], (long)file_size);
        
        printf("Parent: sending file '%s' (size: %ld bytes)\n", 
               filenames[i], (long)file_size);
        
        write(data_write_fd, header, strlen(header) + 1);
        usleep(1000);
        
        while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
            write(data_write_fd, buffer, bytes_read);
            usleep(100);
        }
        
        close(file_fd);
        printf("Parent: file '%s' sent, waiting for ACK...\n", filenames[i]);
        
        read(ack_read_fd, ack, sizeof(ack));
        printf("Parent: received ACK for file '%s'\n", filenames[i]);
    }
    
    char end_msg[20] = "END";
    write(data_write_fd, end_msg, strlen(end_msg) + 1);
    printf("Parent: sent end signal\n");
}


void child_process(int data_read_fd, int ack_write_fd) {
    char buffer[BUFFER_SIZE];
    char filename[MAX_FILENAME];
    char copy_name[MAX_FILENAME];
    int file_fd = -1;
    long file_size = 0;
    int bytes_read;
    int total_bytes = 0;
    int is_receiving = 0;
    char ack[10] = "ACK";
    
    char ready_msg[20] = "READY";
    write(ack_write_fd, ready_msg, strlen(ready_msg) + 1);
    printf("Child: sent ready message\n");
    
    while (1) {
        bytes_read = read(data_read_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            printf("Child: received end of data\n");
            break;
        }
        buffer[bytes_read] = '\0';
        
        if (strncmp(buffer, "FILE:", 5) == 0) {
            if (file_fd >= 0) {
                close(file_fd);
                file_fd = -1;
                is_receiving = 0;
                printf("Child: file '%s' copied (%d bytes)\n", 
                       copy_name, total_bytes);
                write(ack_write_fd, ack, strlen(ack) + 1);
                printf("Child: sent ACK for file '%s'\n", filename);
            }
            
            char *file_part = strstr(buffer, "FILE:") + 5;
            char *size_part = strstr(buffer, "SIZE:");
            
            if (file_part && size_part) {
                char *pipe_pos = strchr(file_part, '|');
                if (pipe_pos) {
                    *pipe_pos = '\0';
                    strcpy(filename, file_part);
                }
                
                sscanf(size_part, "SIZE:%ld", &file_size);
                
                printf("Child: received header for file '%s' (size: %ld bytes)\n", 
                       filename, file_size);
                
                create_copy_name(filename, copy_name);
                
                file_fd = open(copy_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (file_fd == -1) {
                    fprintf(stderr, "Child: error creating file '%s' - %s\n", 
                            copy_name, strerror(errno));
                    continue;
                }
                
                if (file_size == 0) {
                    close(file_fd);
                    file_fd = -1;
                    is_receiving = 0;
                    printf("Child: file '%s' copied (0 bytes)\n", copy_name);
                    write(ack_write_fd, ack, strlen(ack) + 1);
                    printf("Child: sent ACK for file '%s'\n", filename);
                } else {
                    is_receiving = 1;
                    total_bytes = 0;
                }
            }
            continue;
        }
        
        if (strncmp(buffer, "END", 3) == 0) {
            if (file_fd >= 0) {
                close(file_fd);
                printf("Child: file '%s' copied (%d bytes)\n", 
                       copy_name, total_bytes);
                write(ack_write_fd, ack, strlen(ack) + 1);
                printf("Child: sent ACK for file '%s'\n", filename);
            }
            printf("Child: received end signal\n");
            break;
        }
        
        if (is_receiving && file_fd >= 0) {
            long bytes_to_write = file_size - total_bytes;
            if (bytes_to_write <= 0) {
                close(file_fd);
                file_fd = -1;
                is_receiving = 0;
                printf("Child: file '%s' copied (%d bytes)\n", 
                       copy_name, total_bytes);
                write(ack_write_fd, ack, strlen(ack) + 1);
                printf("Child: sent ACK for file '%s'\n", filename);
                continue;
            }
            
            int write_bytes = (bytes_read < bytes_to_write) ? bytes_read : bytes_to_write;
            
            if (write(file_fd, buffer, write_bytes) == -1) {
                fprintf(stderr, "Child: error writing to file\n");
                close(file_fd);
                file_fd = -1;
                is_receiving = 0;
                continue;
            }
            total_bytes += write_bytes;
            
            if (total_bytes >= file_size) {
                close(file_fd);
                file_fd = -1;
                is_receiving = 0;
                printf("Child: file '%s' copied (%d bytes)\n", 
                       copy_name, total_bytes);
                write(ack_write_fd, ack, strlen(ack) + 1);
                printf("Child: sent ACK for file '%s'\n", filename);
            }
        }
    }
    
    if (file_fd >= 0) {
        close(file_fd);
    }
    printf("Child: finished\n");
}
