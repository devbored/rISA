#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdint.h>

#define	syscall_exit            1
#define	syscall_read            4
#define	syscall_write           5

// syscall helper =====================================================================================================

static long syscall(long syscall_type, long arg0, long arg1, long arg2) {
  // Setup argument regs and perform the syscall
  register long a0          asm("a0") = arg0;
  register long a1          asm("a1") = arg1;
  register long a2          asm("a2") = arg2;
  register long syscall_id  asm("a7") = syscall_type;
  asm volatile("scall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(syscall_id));

  // Error handling
  if (a0 < 0) {
    errno = -a0;
    return -1;
  }

  return a0;
}

// Stubs ==============================================================================================================

void _exit(int status) {
  syscall(syscall_exit, status, 0, 0);
  for(;;);
}

ssize_t _write(int file, const void *ptr, size_t len) {
  return syscall(syscall_write, (long)file, (long)ptr, (long)len);
}

ssize_t _read(int file, void *ptr, size_t len) {
  return syscall(syscall_read, (long)file, (long)ptr, (long)len);
}
