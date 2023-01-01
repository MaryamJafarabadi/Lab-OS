#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_find_largest_prime_factor(void)
{
  int n = myproc()->tf->ebp;
  cprintf("We are in kernel mode and the system call of finding the largest prime factor has called so here we are handling it and return appropriate response.\n");
  return find_largest_prime_factor(n);
}

int
sys_get_parent_pid(void)
{
  cprintf("We are in kernel mode and system call of getting parent pid has called so here we are handling it and return appropriate responses!\n");
  return get_parent_pid();
}

int
sys_get_callers(void)
{
  int syscall_number;
  if(argint(0, &syscall_number) < 0)
    return -1;
  return get_callers(syscall_number);
}

int
sys_change_process_queue(void)
{
  int pid, dest_queue;
  if(argint(0, &pid) < 0 || argint(1, &dest_queue) < 0)
    return -1;

  return change_process_queue(pid, dest_queue);
}

int
sys_lottery_ticket(void)
{
  int pid, ticket;
  if(argint(0, &pid) < 0 || argint(1, &ticket) < 0)
    return -1;

  return lottery_ticket(pid, ticket);
}

int
sys_BJF_parameter_process(void)
{
  int pid, priority_ratio, arrivaltime_ratio, execcycle_ratio;
  if(argint(0, &pid) < 0 || argint(1, &priority_ratio) < 0
      || argint(2, &arrivaltime_ratio) < 0 || argint(3, &execcycle_ratio) < 0)
    return -1;

  BJF_parameter_process(pid, priority_ratio, arrivaltime_ratio, execcycle_ratio);
  return 0;
}

int
sys_BJF_parameter_kernel(void)
{
  int priority_ratio, arrivaltime_ratio, execcycle_ratio;
  if(argint(0, &priority_ratio) < 0 || argint(1, &arrivaltime_ratio) < 0 || argint(2, &execcycle_ratio) < 0)
    return -1;

  BJF_parameter_kernel(priority_ratio, arrivaltime_ratio, execcycle_ratio);
  return 0;
}

int
sys_print_information(void)
{
  print_information();
  return 0;
}

int 
sys_sem_init(void)
{
  int i, v;
  if (argint(0, &i) < 0 || argint(1, &v) < 0)
    return -1;

  return sem_init(i, v);
}

int 
sys_sem_acquire(void)
{
  int i;
  if (argint(0, &i) < 0) 
    return -1;

  return sem_acquire(i);
}

int 
sys_sem_release(void)
{
  int i;
  if (argint(0, &i) < 0) 
    return -1;
    
  return sem_release(i);
}
