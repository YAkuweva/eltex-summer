#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <sys/types.h>

int file_exists(const char *filename);
void create_copy_name(const char *original, char *copy);
off_t get_file_size(const char *filename);

#endif
