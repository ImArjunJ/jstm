#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

extern uint32_t _end;

void* _sbrk(ptrdiff_t incr) {
  static unsigned char* heap = (unsigned char*)&_end;
  unsigned char* prev_heap = heap;
  heap += incr;
  return prev_heap;
}

int _close(int fd) {
  (void)fd;
  return -1;
}
int _fstat(int fd, struct stat* st) {
  (void)fd;
  st->st_mode = S_IFCHR;
  return 0;
}
int _isatty(int fd) {
  (void)fd;
  return 1;
}
int _lseek(int fd, int ptr, int dir) {
  (void)fd;
  (void)ptr;
  (void)dir;
  return 0;
}
int _read(int fd, char* ptr, int len) {
  (void)fd;
  (void)ptr;
  (void)len;
  return 0;
}
int _write(int fd, char* ptr, int len) {
  (void)fd;
  (void)ptr;
  (void)len;
  return len;
}
void _exit(int status) {
  (void)status;
  while (1) {
  }
}
int _kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}
int _getpid(void) { return 1; }
