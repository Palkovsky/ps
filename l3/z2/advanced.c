#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/limits.h>
#include <linux/namei.h>
#include <linux/mount.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dawid Macek");
MODULE_DESCRIPTION("Advanced module");

#define MODULE_LOG(fmt, ...) {\
  printk("===== /dev/%s =====", THIS_MODULE->name);  \
  printk(fmt, ##__VA_ARGS__);\
}

static ssize_t jiffies_read(struct file *, char *, size_t, loff_t *);
static ssize_t prname_read(struct file *, char *, size_t, loff_t *);
static ssize_t prname_write(struct file *, const char *, size_t, loff_t *);
static ssize_t mountderef_read(struct file *, char *, size_t, loff_t *);
static ssize_t mountderef_write(struct file *, const char *, size_t, loff_t *);

static const struct file_operations prname_fops = {
 .owner = THIS_MODULE,
 .read  = prname_read,
 .write = prname_write
};

static struct miscdevice prname_device = {
 .minor = MISC_DYNAMIC_MINOR,
 .name  = "prname",
 .fops  = &prname_fops
};

static char prname_buff[TASK_COMM_LEN] = "\x00";

static int prname_resolve(pid_t pid)
{
  struct pid *pid_struct;
  struct task_struct *task;

  pid_struct = find_vpid(pid);
  if(pid_struct == NULL)
    return -EINVAL;

  task = pid_task(pid_struct, PIDTYPE_PID);
  if(task == NULL)
    return -EINVAL;

  if(get_task_comm(prname_buff, task) == NULL)
    return -EINVAL;

  return 0;
}

static ssize_t prname_read(struct file *filep, char *buff, size_t count, loff_t *offp)
{
  int result;
  ssize_t kstrsz;
  ssize_t written;
  ssize_t left;

  kstrsz = strlen(prname_buff)+1;
  written = *offp;
  left = kstrsz-written;

  if(left > count)
    left = count;

  MODULE_LOG("prname_read: %ld | %ld | %ld\n", kstrsz, written, left);
  if(left <= 0)
    return 0;

  result = copy_to_user(buff, prname_buff+written, left);
  if(result != 0) {
    MODULE_LOG("prname_read: error copying to user-space %d\n", result);
    return -EFAULT;
  }

  *offp += left;
  return left;
}

static ssize_t prname_write(struct file *filep, const char *buff, size_t count, loff_t *offp)
{
  int result;
  pid_t pid;
  char kbuff[32];

  MODULE_LOG("prname_write: off: %lld, count: %ld...\n", *offp, count);
  if(*offp > 0 || count > 32)
    return 0;

  result = copy_from_user(kbuff, buff, count);
  if(result != 0) {
    MODULE_LOG("prname_write: error copying from user-space\n");
    return -EFAULT;
  }

  result = kstrtou32(kbuff, 10, &pid);
  if(result != 0) {
    MODULE_LOG("prname_write: unable to parse PID\n");
    return result;
  } else if(pid == 0) {
    MODULE_LOG("prname_write: PID of 0 is not allowed\n");
    return -EINVAL;
  }

  result = prname_resolve(pid);
  if(result != 0) {
    MODULE_LOG("prname_write: invalid PID\n");
    return result;
  } else {
    MODULE_LOG("prname_write: Resolved '%s'\n", prname_buff);
  }

  *offp += count;
  return count;
}

static const struct file_operations jiffies_fops = {
 .owner = THIS_MODULE,
 .read  = jiffies_read,
};

static struct miscdevice jiffies_device = {
 .minor = MISC_DYNAMIC_MINOR,
 .name  = "jiffies",
 .fops  = &jiffies_fops
};

static ssize_t jiffies_read(struct file *filep, char *buff, size_t count, loff_t *offp)
{
  int result;
  u64 jiffies;
  size_t kstrsz;
  char kbuff[32];

  if(*offp > 0)
    return 0;

  jiffies = get_jiffies_64();
  result = sprintf(kbuff, "%lld", jiffies);
  if(result < 0) {
    MODULE_LOG("jiffies_read: unable to access jiffies\n");
    return -EFAULT;
  }

  kstrsz = strlen(kbuff);
  result = copy_to_user(buff, kbuff, kstrsz);
  if(result != 0) {
    MODULE_LOG("jiffies_read: error copying to user-space\n");
    return -EFAULT;
  }

  *offp = kstrsz;
  return kstrsz;
}

static const struct file_operations mountderef_fops = {
 .owner = THIS_MODULE,
 .read  = mountderef_read,
 .write  = mountderef_write
};

static struct miscdevice mountderef_device = {
 .minor = MISC_DYNAMIC_MINOR,
 .name  = "mountderef",
 .fops  = &mountderef_fops
};

static char mountderef_buff[PATH_MAX+1] = "\x00";
static char *mountderef_ptr = NULL;

static ssize_t mountderef_read(struct file *filep, char *buff, size_t count, loff_t *offp)
{
  int result;
  ssize_t kstrsz;
  ssize_t written;
  ssize_t left;

  if(mountderef_ptr == NULL)
    return -EINVAL;

  written = *offp;
  kstrsz = strlen(mountderef_ptr);
  left = kstrsz-written;
  if(left > count)
    left = count;

  if(left <= 0)
    return 0;

  result = copy_to_user(buff, mountderef_ptr, left);
  if(result != 0) {
    MODULE_LOG("mountderef_read: error copying to user-space\n");
    return -EFAULT;
  }

  *offp += left;
  return left;
}

static ssize_t mountderef_write(struct file *filep, const char *buff, size_t count, loff_t *offp)
{
  int result;
  ssize_t read;
  ssize_t left;
  struct path path;

  read = *offp;
  left = count-read;
  if(read + left > PATH_MAX)
    left = PATH_MAX-read;

  result = copy_from_user(mountderef_buff+read, buff, left);
  if(result != 0) {
    MODULE_LOG("iter: error copying from user-space\n");
    return -EFAULT;
  }

  *offp += left;

  //  Path lookup and mount point resultion after write done.
  if(*offp == count) {
    mountderef_buff[count] = 0;
    MODULE_LOG("mountderef_wrie: Finished read. Got '%s'\n", mountderef_buff);

    result = kern_path(mountderef_buff, LOOKUP_FOLLOW, &path);
    if(result != 0)
      return -EINVAL;

    // Mountpoint dereference doesn't really work.
    mountderef_ptr = dentry_path_raw(path.mnt->mnt_root,
                                     mountderef_buff, PATH_MAX);
    if(mountderef_ptr == NULL)
      return -EINVAL;

    MODULE_LOG("mountderef_write: mnt %s | %s | %s | %s\n",
               path.mnt->mnt_sb->s_id,
               path.mnt->mnt_root->d_name.name,
               path.dentry->d_name.name,
               mountderef_ptr);
  }

  return left;
}

static int __init advanced_init(void)
{
  int result = 0;

  result = misc_register(&prname_device);
  if(result != 0) {
    MODULE_LOG("unable to register prname device. code: %d\n", result);
    goto out_deregister;
  }

  result = misc_register(&jiffies_device);
  if(result != 0) {
    MODULE_LOG("unable to register jiffies device. code: %d\n", result);
    goto out_deregister;
  }

  result = misc_register(&mountderef_device);
  if(result != 0) {
    MODULE_LOG("unable to register mountderef device. code: %d\n", result);
    goto out_deregister;
  }

  MODULE_LOG("load ok\n");
  return result;

 out_deregister:
  misc_deregister(&prname_device);
  misc_deregister(&jiffies_device);
  misc_deregister(&mountderef_device);
  return result;
}

static void __exit advanced_exit(void)
{
  misc_deregister(&prname_device);
  misc_deregister(&jiffies_device);
  misc_deregister(&mountderef_device);
  MODULE_LOG("unload ok\n");
}

module_init(advanced_init);
module_exit(advanced_exit);
