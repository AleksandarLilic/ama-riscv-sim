#ifndef NEWLIB_DEFS_H
#define NEWLIB_DEFS_H

#include <sys/times.h>
#include <sys/stat.h>
#include <errno.h>
#undef errno
extern int errno;

#include "mem_map.h"
#include "common.h"

// subroutines as per https://sourceware.org/newlib/libc.html#Syscalls

void _exit(int i) {
    (void)i;
    asm volatile ("ecall");
}

int _close(int file) {
    (void)file;
    return -1;
}

char *__env[1] = { 0 };
char **environ = __env;

int _execve(char* name, char** argv, char** env) {
    (void)name;
    (void)argv;
    (void)env;
    errno = ENOMEM;
    return -1;
}

int _fork(void) {
    errno = EAGAIN;
    return -1;
}

int _fstat(int file, struct stat* st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _getpid(void) {
    return 1;
}

void _kill(int pid) {
    (void)pid;
    errno = EINVAL;
    asm volatile ("ecall");
}

int _link(char* old, char* new) {
    (void)old;
    (void)new;
    errno = EMLINK;
    return -1;
}

int _lseek(int file, int ptr, int dir) {
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _isatty(int file) {
    (void)file;
    return 1;
}

int _open(const char* name, int flags, int mode) {
    (void)name;
    (void)flags;
    (void)mode;
    return -1;
}

int _read(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

extern uint32_t _end; // defined in linker script as the end of the BSS section
void* _sbrk(int incr) {
    static uint32_t *heap = NULL;
    uint32_t *prev_heap;
    if (heap == NULL)
        heap = (uint32_t *)&_end;
    prev_heap = heap;
    heap += incr;
    return prev_heap;
}

int _stat(char* file, struct stat* st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

clock_t times(struct tms* buf) {
    if (!buf) {
        errno = EINVAL;
        return -1;
    }
    buf->tms_utime = get_cpu_time(); // user time
    buf->tms_stime = 0; // system time
    buf->tms_cutime = 0; // child user time
    buf->tms_cstime = 0; // child system time
    return buf->tms_utime;
}

int _unlink(char* name) {
    (void)name;
    errno = ENOENT;
    return -1;
}

int _wait(int* status) {
    (void)status;
    errno = ECHILD;
    return -1;
}

// int _write(int file, char *ptr, int len) defined in common.c

#endif
