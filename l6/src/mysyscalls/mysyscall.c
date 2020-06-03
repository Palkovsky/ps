#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE0(mysyscall)
{
  printk(KERN_EMERG "sys_mysyscall\n");
  return 0;
}


