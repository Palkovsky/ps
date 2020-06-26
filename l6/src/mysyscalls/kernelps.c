#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/rcupdate.h>

#define BUFFSZ TASK_COMM_LEN

static inline size_t taskcount(void)
{
  size_t count = 0;
  struct task_struct *task;
  for_each_process(task) {
    count++;
  }
  return count;
}

SYSCALL_DEFINE2(kernelps, size_t __user*, count, char __user*, buffer)
{
  struct task_struct *task;
  size_t bytes_avail, bytes_required, written=0;
  long err = 0;

  if(!count)
    return -EFAULT;

  rcu_read_lock();
  bytes_avail = *count;
  bytes_required = taskcount()*BUFFSZ;

  if(!buffer) {
    err = copy_to_user(count, &bytes_required, sizeof(size_t));
    if(err)
      printk(KERN_EMERG "[sys_kernelps] Unable to copy required size to userspace.\n");
    goto ret;
  }

  if(bytes_required > bytes_avail) {
    printk(KERN_EMERG "[sys_kernelps] Buffer to small. Got %ld bytes, but %ld required.\n",
           bytes_avail, bytes_required);
    err = -ENOMEM;
    goto ret;
  }

  for_each_process(task) {
    char *taskname = (char*) &task->comm;
    size_t offset = (written++)*BUFFSZ;

    err = copy_to_user(buffer+offset, taskname, BUFFSZ);
    if(err)
      goto ret;
  }

  err = copy_to_user(count, &written, sizeof(size_t));
  if(err)
    printk(KERN_EMERG "[sys_kernelps] Unable to copy number of bytes written.\n");

 ret:
  rcu_read_unlock();
  return err;
}
