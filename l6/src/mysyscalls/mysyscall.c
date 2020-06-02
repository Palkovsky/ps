#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage long sys_mysyscall(void)
{
  printk (KERN_EMERG "Hura! Moge napisac 'Hello world' z wnetrza jadra!\n");
  return 0;
}
