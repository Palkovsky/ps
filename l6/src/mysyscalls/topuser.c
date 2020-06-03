#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>

#include "uidtask.h"

static long topuser_present(struct uidtask *key, struct task_struct *task)
{
  key->acc++;
  return 0;
}

static long topuser_fold(uid_t *uid)
{
  long bkt;
  uid_t result = 0;
  u64 count = 0;
  struct uidtask *it;

  // TODO: Emptiness check

  hash_for_each(UIDTASK_LOOKUP, bkt, it, node) {
    printk(KERN_EMERG "[topuser] UID: %d -> %lld\n", it->uid, it->acc);
    if(it->acc >= count) {
      result = it->uid;
      count = it->acc;
    }
  }

  *uid = result;
  return 0;
}

const static struct uidtask_lookup_conf conf =
  {
   .present = topuser_present,
   .fold = topuser_fold
  };


SYSCALL_DEFINE1(topuser, uid_t __user*, uid)
{
  uid_t result;
  long err = 0;

  if((err = uidtask_lookup(&conf, &result))) {
    printk(KERN_EMERG "[sys_topuser]: Unable to find UID..\n");
    err = -EFAULT;
    goto ret;
  }

  if(copy_to_user(uid, &result, sizeof(uid_t))) {
    printk(KERN_EMERG "[sys_topuser]: Unable to copy.\n");
    err = -EFAULT;
  }

 ret:
  return err;
}
