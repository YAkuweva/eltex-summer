#include "file_utils.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

void create_copy_name(const char *original, char *copy) {
    sprintf(copy, "%s.copy", original);
}

off_t get_file_size(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        return -1;
    }
    
    off_t size = lseek(fd, 0, SEEK_END);
    close(fd);
    return size;
}
