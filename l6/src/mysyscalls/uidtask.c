#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/rcupdate.h>
#include <linux/mutex.h>
#include <linux/syscalls.h>

#include "uidtask.h"

static DEFINE_MUTEX(LOOKUP_MUTEX);
DEFINE_HASHTABLE(UIDTASK_LOOKUP, UIDTASK_BITS);

long uidtask_lookup(const struct uidtask_lookup_conf *config, uid_t *result)
{
  long err = 0;
  struct task_struct *task;
  u64 bkt;
  struct uidtask *it;

  mutex_lock(&LOOKUP_MUTEX);

  rcu_read_lock();
  for_each_process(task) {
    uid_t procuid = task->cred->uid.val;
    struct uidtask *keyval = NULL;

    // Check for existing keys.
    hash_for_each_possible(UIDTASK_LOOKUP, it, node, procuid) {
      if(it->uid == procuid) {
        keyval = it;
        config->present(it, task);
        break;
      }
    }

    if(keyval)
      continue;

    keyval = kmalloc(sizeof(struct uidtask), GFP_KERNEL);
    if(!keyval) {
      err = -ENOMEM;
      goto cleanup;
    }
    keyval->uid = procuid;
    keyval->acc = 0;
    config->present(keyval, task);

    // Add node
    hash_add(UIDTASK_LOOKUP, &keyval->node, procuid);
  }
  rcu_read_unlock();

  // Filter data to find uid.
  if(config->fold(result))
    err = -EFAULT;

 cleanup:
  hash_for_each(UIDTASK_LOOKUP, bkt, it, node) {
    hash_del(&it->node);
    kfree(it);
  }
  mutex_unlock(&LOOKUP_MUTEX);
  return err;
}
