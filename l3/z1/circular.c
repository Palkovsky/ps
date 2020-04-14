#include <stdarg.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dawid Macek");
MODULE_DESCRIPTION("Circular buffer");

#define CIRCULAR_BUFFSZ 20
#define CIRCULAR_LOG(fmt, ...) {\
  printk("===== /dev/%s =====", THIS_MODULE->name);  \
  printk(fmt, ##__VA_ARGS__);\
}

static ssize_t circular_read(struct file *filep, char *buff, size_t count, loff_t *offp)
{
  CIRCULAR_LOG("%s", "circular_read\n");
  return 0;
}

static ssize_t circular_write(struct file *filep, const char *buff, size_t count, loff_t *offp)
{
  CIRCULAR_LOG("%s", "circular_write\n");
  return 0;
}

static ssize_t circular_proc_write(struct file *filep, const char *buff, size_t count, loff_t *offp)
{
  CIRCULAR_LOG("circular_proc_write\n");
  return 0;
}

static ssize_t circular_proc_read(struct file *filep, char *buff, size_t count, loff_t *offp)
{
  CIRCULAR_LOG("circular_proc_read\n");
  return 0;
}

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

static int __init circular_init(void)
{
  int result = 0;
  struct proc_dir_entry* proc_entry;

  result = misc_register(&circular_device);
  if(result != 0) {
    CIRCULAR_LOG("misc_register() failed with %d\n", result);
    return result;
  }

  proc_entry = proc_create(THIS_MODULE->name, 0, NULL, &circular_proc_ops);
  if(!proc_entry) {
    CIRCULAR_LOG("proc_entry() has failed\n");
    result = 1;
    goto out_deregister;
  }

  CIRCULAR_LOG("load ok\n");
  return result;

 out_deregister:
  misc_deregister(&circular_device);
  return result;
}

static void __exit circular_exit(void)
{
  misc_deregister(&circular_device);
  remove_proc_entry(THIS_MODULE->name, NULL);
  CIRCULAR_LOG("module removed\n");
}

module_init(circular_init);
module_exit(circular_exit);
