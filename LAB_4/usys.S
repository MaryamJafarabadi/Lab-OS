#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(find_largest_prime_factor)
SYSCALL(change_file_size)
SYSCALL(get_parent_pid)
SYSCALL(get_callers)
SYSCALL(change_process_queue)
SYSCALL(lottery_ticket)
SYSCALL(BJF_parameter_process)
SYSCALL(BJF_parameter_kernel)
SYSCALL(print_information)
SYSCALL(sem_acquire)
SYSCALL(sem_release)
SYSCALL(sem_init)
