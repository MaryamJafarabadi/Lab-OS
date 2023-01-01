#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

int rand_number(int n)
{
  int random;
  acquire(&tickslock);
  random = (ticks * 12456) % n;
  random = (random * ticks) % n;
  release(&tickslock);
  return random + 1;
}

float get_priority(int n)
{
  return 1/n;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->level = 2;
  p->cycle = 0;
  p->wait = 0;
  p->ticket = rand_number(100);
  p->priority = get_priority(p->ticket);
  acquire(&tickslock);
  p->arrivaltime = ticks;
  release(&tickslock);
  p->execcycle = 0;
  p->cpu_time = 0;
  p->priority_ratio = 1;
  p->arrivaltime_ratio = 1;
  p->execcycle_ratio = 1;
  
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

struct proc* round_robin(void)
{
  struct proc *p, *final= 0;
  int now = ticks;
  int max_proc = -1000000;
  //int proc_available = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->state != RUNNABLE || p->level != 1)
      continue;

    if(now - p->cpu_time > max_proc)
    {
      final = p;
      max_proc = now - p->cpu_time;
    }
    else
      continue;
  }
  return final;
}

struct proc* get_lottery(void)
{
  struct proc *p;
  struct proc *final = 0;
  int total_ticket = 0;
  int rand_ticket = 0;
  int current_ticket = 0;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->state != RUNNABLE || p->level != 2)
      continue;
    total_ticket += p->ticket;
  }

  if(total_ticket == 0)
    return 0;

  rand_ticket = ticks % total_ticket;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->state != RUNNABLE || p->level != 2)
      continue;
    current_ticket += p->ticket;

    if(rand_ticket < current_ticket)
    {
      final = p;
      break;
    }
  }
  return final;
}

struct proc* best_job_first(void)
{
  struct proc* p, *final = 0;
  float rank = 0;
  float min_rank = 10000000;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state != RUNNABLE || p->level != 3)
      continue;
    rank = p->priority * p->priority_ratio + p->arrivaltime * p->arrivaltime_ratio + p->execcycle * p->execcycle_ratio;
    if(rank < min_rank)
    {
      final = p;
      min_rank = rank;
    }
  }
  return final;
}

void agging()
{
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE)
      continue;
    if (p->wait >= 8000)
    {
        p->level = 1;
        p->wait = 0;
    }
  }
}

void waiting()
{
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->state != RUNNABLE)
      continue;
    p->wait++;
  }
}
//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;)
  {
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    p = round_robin();

    if(p == 0)
      p = get_lottery();

    if(p == 0)
      p = best_job_first();

    if(p != 0)
    {
      p->cycle++;
      c->proc = p;
      switchuvm(p);

      p->state = RUNNING;
      p -> execcycle += 0.1;

      waiting();
      p->wait = 0;
      agging();
      swtch(&(c->scheduler), p->context);
      switchkvm();
      c->proc = 0;
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  myproc()->cpu_time = ticks;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int
find_largest_prime_factor(int n)
{
  int divisor = 2, result = 0;
  if (n < 1) return -2;
  if (n == 1) return -1;
  while(n != 0)
  {
    if(n == 1) return result;
    else if(n % divisor == 0)
    {
      result = n;
      n = n / divisor;
    }
    else divisor++;
  }
  return 0;
}

int get_parent_pid(void)
{
  return myproc()->parent->pid;
}

int callers[SYSCALL_NUM][PIDS_NUM]={0};

void
add_this_pid(int syscall_number, int pid)
{
  callers[syscall_number][callers[syscall_number][SIZE]] = pid;
  callers[syscall_number][SIZE]++;
}

void
print_all_pids(int syscall_number, int size)
{
  cprintf("%d", callers[syscall_number][0]);
  for(int i = 1; i < size; i++)
  {
    cprintf(", %d",callers[syscall_number][i]);
  }
  cprintf("\n");
}

int
get_callers(int syscall_number)
{
  int size = callers[syscall_number][SIZE];
  if(size == 0)
  {
    cprintf("There is no process available which has called this system call!\n");
    return 0;
  }
  else
  if(size > PIDS_NUM - 1)
    size = PIDS_NUM - 1;

  print_all_pids(syscall_number, size);
  return 0;
}
int
change_process_queue(int pid, int dest_queue)
{
  struct proc *p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid)
    {
      p->wait = 0;
      p->level = dest_queue;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

int
lottery_ticket(int pid, int ticket)
{
  struct proc* p;
  acquire(&ptable.lock);
  for (p = ptable.proc; p< &ptable.proc[NPROC]; p++)
  {
    if(p->pid == pid)
    {
      p->ticket = ticket;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

void
BJF_parameter_process(int pid, int priority_ratio, int arrivaltime_ratio, int execcycle_ratio)
{
  struct proc* p;
  acquire(&ptable.lock);
  for (p = ptable.proc; p< &ptable.proc[NPROC]; p++)
  {
    if(p->pid == pid)
    {
      p->priority_ratio = (float)priority_ratio;
      p->arrivaltime_ratio = (float)arrivaltime_ratio;
      p->execcycle_ratio = (float)execcycle_ratio;
    }
  }
  release(&ptable.lock);
}

void
BJF_parameter_kernel(int priority_ratio, int arrivaltime_ratio, int execcycle_ratio)
{
  struct proc* p;
  for (p = ptable.proc; p< &ptable.proc[NPROC]; p++)
  {
    p->priority_ratio = (float)priority_ratio;
    p->arrivaltime_ratio = (float)arrivaltime_ratio;
    p->execcycle_ratio = (float)execcycle_ratio;
  }
}

void print_space(int used_length, int total_space)
{
  int available_space = total_space - used_length;
  if(available_space>0)
  {
    for(int i=0;i<available_space;i++)
    {
      cprintf(" ");
    }
  }
}

int nDigits(int i)
{
  if (i < 0) i = -i;
  if (i <         10) return 1;
  if (i <        100) return 2;
  if (i <       1000) return 3;
  if (i <      10000) return 4;
  if (i <     100000) return 5;
  if (i <    1000000) return 6;
  if (i <   10000000) return 7;
  if (i <  100000000) return 8;
  if (i < 1000000000) return 9;
  return 10;
}

int string_compare(char str1[], char str2[])
{
    int ctr=0;

    while(str1[ctr]==str2[ctr])
    {
        if(str1[ctr]=='\0' && str2[ctr]=='\0')
            break;
        ctr++;
    }
    if(str1[ctr]=='\0' && str2[ctr]=='\0')
        return 0;
    else
        return -1;
}

void print_information()
{
  static char *states[] = {
    [UNUSED]    "UNUSED",
    [EMBRYO]    "EMBRYO",
    [SLEEPING]  "SLEEPING",
    [RUNNABLE]  "RUNNABLE",
    [RUNNING]   "RUNNING",
    [ZOMBIE]    "ZOMBIE"
  };
  struct proc* p;
  int x = 17;
  cprintf("name            pid    state    queue-level    arrivaltime    ticket    P_R    A_R    E_R    rank    cycle\n");
  cprintf("--------------------------------------------------------------------------------------------------------------\n");
  //acquire(&ptable.lock);
  for (p = ptable.proc; p< &ptable.proc[NPROC]; p++)
  {
    x = 17;
    if(strlen(p->name) == 0)
      continue;

    int rank = (int) p->priority * p->priority_ratio + p->arrivaltime * p->arrivaltime_ratio + p->execcycle * p->execcycle_ratio;
    char* state = states[p->state];


    cprintf("%s", p->name);
    if(string_compare(p->name, "init") == 0  && string_compare(p->name, "sh") == 0)
      x = 15;
    int l = strlen(p->name);
    print_space(l,16);

    cprintf("%d", p->pid);
    l = nDigits(p->pid);
    print_space(l,7);//10

    cprintf("%s", state);
    l = strlen(state);
    print_space(l,9);

    cprintf("%d", p->level);
    l = nDigits(p->level);
    print_space(l,15);

    cprintf("%d", p->arrivaltime);
    l = nDigits(p->arrivaltime);
    print_space(l,x);

    cprintf("%d", p->ticket);
    l = nDigits(p->ticket);
    print_space(l,10);

    cprintf("%d", (int) p->priority_ratio);
    l = nDigits((int)p->priority_ratio);
    print_space(l,7);

    cprintf("%d", (int) p->arrivaltime_ratio);
    l = nDigits((int)p->arrivaltime_ratio);
    print_space(l,7);

    cprintf("%d", (int) p->execcycle_ratio);
    l = nDigits((int)p->execcycle_ratio);
    print_space(l,7);

    cprintf("%d", rank);
    l = nDigits(rank);
    print_space(l,8);

    cprintf("%d\n", p->cycle);

  }
  //release(&ptable.lock);
}

typedef struct
{
  int value;
  int last_index;
  struct proc *queue[NPROC];
  struct spinlock lock;
} Semaphore;

Semaphore semaphore[5];

int sem_init(int i, int v)
{
  initlock(&semaphore[i].lock, "semaphore");
  semaphore[i].value = v;
  semaphore[i].last_index = 0;
  return 0;
}

int sem_acquire(int i)
{
  acquire(&semaphore[i].lock);
  if (semaphore[i].value <= 0)
  {
    struct proc *p = myproc();
    semaphore[i].queue[semaphore[i].last_index] = p;
    semaphore[i].last_index++;
    sleep(&semaphore, &semaphore[i].lock);
  }
  else
  {
    semaphore[i].value--;
  }
  if (i != 5)
  {
    cprintf("philosopher: %d acquired %d\n", myproc()->pid - 3, i);
  }
  release(&semaphore[i].lock);
  return 0;
}

int sem_release(int i)
{
  if (semaphore[i].last_index > 0)
  {
    semaphore[i].last_index--;
    struct proc *p = semaphore[i].queue[semaphore[i].last_index];
    acquire(&ptable.lock);
    p->state = RUNNABLE;
    release(&ptable.lock);
  }
  else
  {
    semaphore[i].value++;
  }
  if (i != 5)
  {
    cprintf("philosopher: %d released %d\n", myproc()->pid - 3, i);
  }
  return 0;
}
