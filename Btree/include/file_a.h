#ifndef FILE_A_H
#define FILE_A_H

#include <stdio.h>

#ifdef __cplusplus
extern "C"{
#endif

struct file_a;
typedef struct file_a file_a;

file_a* file_create(const char* name, const char* mode);

void file_destroy(file_a** f);

#define filelocal __attribute__((cleanup(file_destroy)))

void file_lock(file_a* f);

void file_unlock(file_a* f);

int file_seek(file_a* f, off_t offset);

int file_seek_relative(file_a* f, off_t offset, int from);

int file_size(file_a* f);

size_t file_read(file_a* f, char* data, size_t size);

size_t file_write(file_a* f, const char* data, size_t size);

off_t file_tell(file_a* f);

int file_get_delim(file_a* f, char* buff, int len, char delim);

#ifdef __cplusplus
}
#endif

#endif //FILE_A_H
