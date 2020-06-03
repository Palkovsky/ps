#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE2(pidtoname, pid_t, pid, char __user*, name)
{
  printk (KERN_EMERG "sys_pidtoname %d\n", pid);
  return 0;
}
