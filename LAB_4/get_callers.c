#include "types.h"
#include "stat.h"
#include "user.h"
#include "syscall.h"

int main(int argc, char* argv[])
{
  printf(1, "The result of SYS_write:\n");
  get_callers(SYS_write);
  printf(1, "The result of SYS_fork:\n");
  get_callers(SYS_fork);
  printf(1, "The result of SYS_wait:\n");
  get_callers(SYS_wait);
  exit();
}
