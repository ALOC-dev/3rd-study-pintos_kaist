#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>
#include "filesys/file.h"

void syscall_init (void);

#endif /* userprog/syscall.h */
void check_address(void *addr);
void halt(void);
void exit(int status);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);

int open(const char *file);
int write(int fd, const void *buffer, unsigned size);
int read(int fd, void *buffer, unsigned size);
int filesize(int fd);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);

int process_add_file(struct file *f);
struct file *process_get_file(int fd);