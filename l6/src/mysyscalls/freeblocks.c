#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE2(freeblocks, char __user *, filepath, u64 __user*, avail)
{
  char buff[256];
  if(copy_from_user(buff, filepath, sizeof(buff))) {
    printk (KERN_EMERG "sys_freeblocks: Unable to copy.\n");
    return -EFAULT;
  }
  printk (KERN_EMERG "sys_freeblocks: %s\n", buff);
  return 0;
}
