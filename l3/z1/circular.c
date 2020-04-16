#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dawid Macek");
MODULE_DESCRIPTION("Circular buffer");

#define MODULE_LOG(fmt, ...) {\
  printk("===== /dev/%s =====", THIS_MODULE->name);  \
  printk(fmt, ##__VA_ARGS__);\
}

static ssize_t circular_read(struct file *, char *, size_t, loff_t *);
static ssize_t circular_write(struct file *, const char *, size_t, loff_t *);
static ssize_t circular_proc_write(struct file *, const char *, size_t, loff_t *);
static ssize_t circular_proc_read(struct file *, char *, size_t, loff_t *);

static uint64_t CIRCULAR_SZ = 64;
static uint64_t circular_index = 0;
static char* circular_buff = NULL;

static struct proc_ops circular_proc_ops = {
 .proc_read     = circular_proc_read,
 .proc_write    = circular_proc_write,
};

static const struct file_operations device_fops = {
 .owner = THIS_MODULE,
 .read  = circular_read,
 .write = circular_write
};

static struct miscdevice circular_device = {
 .minor = MISC_DYNAMIC_MINOR,
 .name  = THIS_MODULE->name,
 .fops  = &device_fops
};

static int circular_realloc(void)
{
  if(circular_buff == NULL) {
    circular_buff = (char*) kmalloc(CIRCULAR_SZ, GFP_KERNEL);
  } else {
    circular_buff = (char*) krealloc(circular_buff, CIRCULAR_SZ, GFP_KERNEL);
  }

  if(circular_buff == NULL) {
    return -ENOMEM;
  }

  memset(circular_buff, 0, CIRCULAR_SZ);
  circular_index = 0;

  return 0;
}

static int circular_realease(void)
{
  kfree(circular_buff);
  return 0;
}

static ssize_t circular_read(struct file *filep, char *buff, size_t count, loff_t *offp)
{
  int result;

  if(*offp > 0 || count < CIRCULAR_SZ) {
    return 0;
  }

  result = copy_to_user(buff, circular_buff, CIRCULAR_SZ);
  if(result != 0) {
    MODULE_LOG("circular_read: error copying to user-space\n");
    return -EFAULT;
  }

  *offp += CIRCULAR_SZ;
  return CIRCULAR_SZ;
}

static ssize_t circular_write(struct file *filep, const char *buff, size_t count, loff_t *offp)
{
  int i;
  int result;
  char kbuff[128];
  ssize_t kbuffsz = 0;
  ssize_t bytes_read = 0;

  while(bytes_read < count) {
    kbuffsz = ((count-bytes_read) > 128) ? 128 : count-bytes_read;

    result = copy_from_user(kbuff, buff+bytes_read, kbuffsz);
    if(result != 0) {
      MODULE_LOG("circular_write: error copying from user-space\n");
      return -EFAULT;
    }

    for(i=0; i<kbuffsz; i++) {
      circular_buff[circular_index] = kbuff[i];
      circular_index = (circular_index+1) % CIRCULAR_SZ;
    }

    bytes_read += kbuffsz;
  }

  return count;
}

static ssize_t circular_proc_write(struct file *filep, const char *buff, size_t count, loff_t *offp)
{
  int result;
  unsigned long parsed;
  char kbuff[32];

  MODULE_LOG("circular_proc_write: off: %lld, count: %ld...\n", *offp, count);
  if(*offp > 0 || count > 32) {
    return 0;
  }

  result = copy_from_user(kbuff, buff, count);
  if(result != 0) {
    MODULE_LOG("circular_proc_write: error copying from user-space\n");
    return -EFAULT;
  }

  result = kstrtoul(kbuff, 10, &parsed);
  if(result != 0) {
    MODULE_LOG("circular_proc_write: parsing error\n");
    return result;
  }

  if(parsed == 0) {
    MODULE_LOG("circular_proc_write: 0 is not allowed\n");
    return -EINVAL;
  }

  *offp += count;
  CIRCULAR_SZ = (uint64_t) parsed;

  result = circular_realloc();
  if(result != 0) {
    MODULE_LOG("circular_proc_write: unable to realloc buffer\n");
    return result;
  }

  return count;
}

static ssize_t circular_proc_read(struct file *filep, char *buff, size_t count, loff_t *offp)
{
  int result;
  size_t kstrsz;
  char kbuff[32];

  result = sprintf(kbuff, "%lld", CIRCULAR_SZ);
  if(result < 0) {
    MODULE_LOG("circular_proc_read: unable to parse BUFF_SIZE\n");
    return -EFAULT;
  }

  kstrsz = strlen(kbuff);
  if(*offp > 0 || count < kstrsz) {
    return 0;
  }

  result = copy_to_user(buff, kbuff, kstrsz);
  if(result != 0) {
    MODULE_LOG("circular_proc_read: error copying to user-space\n");
    return -EFAULT;
  }

  *offp = kstrsz;
  return kstrsz;
}

static int __init circular_init(void)
{
  int result = 0;
  struct proc_dir_entry* proc_entry;

  result = misc_register(&circular_device);
  if(result != 0) {
    MODULE_LOG("misc_register() failed with %d\n", result);
    return result;
  }

  proc_entry = proc_create(THIS_MODULE->name, 0777, NULL, &circular_proc_ops);
  if(!proc_entry) {
    MODULE_LOG("proc_entry() has failed\n");
    result = 1;
    goto out_deregister;
  }

  result = circular_realloc();
  if(result != 0) {
    MODULE_LOG("circular_realloc() failed\n");
    goto out_remove_proc;
  }

  MODULE_LOG("load ok\n");
  return result;

 out_remove_proc:
  remove_proc_entry(THIS_MODULE->name, NULL);
 out_deregister:
  misc_deregister(&circular_device);
  return result;
}

static void __exit circular_exit(void)
{
  misc_deregister(&circular_device);
  remove_proc_entry(THIS_MODULE->name, NULL);
  circular_realease();
  MODULE_LOG("unload ok\n");
}

module_init(circular_init);
module_exit(circular_exit);
