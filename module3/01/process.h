#ifndef PROCESS_H
#define PROCESS_H


void parent_process(int data_write_fd, int ack_read_fd, char **filenames, int file_count);

void child_process(int data_read_fd, int ack_write_fd);

#endif
