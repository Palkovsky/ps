#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>

SYSCALL_DEFINE2(pidtoname, pid_t, pid, char __user*, buffer)
{
  long err = 0;
  char *procname = NULL;
  struct task_struct *task;

  for_each_process(task) {
    if(task->pid == pid) {
      procname = (char*) &task->comm;
      break;
    }
  }

  if(!procname) {
    printk(KERN_EMERG "[sys_pidtoname] Invalid PID.\n");
    return -ESRCH;
  }

  err = copy_to_user(buffer, procname, TASK_COMM_LEN);
  if(err)
    printk(KERN_EMERG "[sys_pidtoname] Error while copying procname to userspace.\n");

  return err;
}
