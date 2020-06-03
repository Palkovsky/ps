#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE2(kernelps, size_t __user*, count, char __user*, procs)
{
  printk (KERN_EMERG "sys_kernelps\n");
  return 0;
}
