#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/rcupdate.h>

MODULE_LICENSE("GPL");

const struct file_operations simple_fops;

const int simple_major = 198;

const char msg_str[] = "-0123456789-ABCDEFGHIJ-";
const int msg_len = sizeof(msg_str);

static int *msg_pos = NULL;

static int __init simple_init(void)
{
	int result;

  msg_pos = kmalloc(sizeof(int), GFP_KERNEL);
  if(!msg_pos) {
    printk(KERN_ERR "SIMPLE: Unable to allocate msg_pos_ptr.\n");
    return EFAULT;
  }
  *msg_pos = 0;

	result = register_chrdev(simple_major, "simple", &simple_fops);
	if (result < 0) {
		printk(KERN_ERR "SIMPLE: cannot register the /dev/simple\n");
		return result;
	}

	printk(KERN_INFO "SIMPLE: module has been inserted.\n");
	return 0;
}

static void __exit simple_exit(void)
{
  kfree(msg_pos);
	unregister_chrdev(simple_major, "simple");

	printk(KERN_INFO "SIMPLE: module has been removed\n");
}

ssize_t simple_read(struct file *filp, char __user *user_buf,
	size_t count, loff_t *f_pos) {
	char *local_buf;
	int length_to_copy;
	int i;
	int err;
  int last_pos;
  int *new_pos;

	// 1. Prepare the text to send
	// Calculate the length
  rcu_read_lock();
	length_to_copy = msg_len - (*rcu_dereference(msg_pos) % msg_len);
  rcu_read_unlock();

	if (length_to_copy > count)
		length_to_copy = count;

	local_buf = kmalloc(length_to_copy, GFP_KERNEL);
	if (!local_buf) {
		err = -ENOMEM;
		goto cleanup;
	}

  // Perform reading(without copying msg_pos onto the stack).
  rcu_read_lock();
  new_pos = rcu_dereference(msg_pos);
	for (i = 0; i < length_to_copy; i++) {
		local_buf[i] = msg_str[(*new_pos + i) % msg_len];
		msleep(100);
	}
  last_pos = *new_pos + length_to_copy;
  rcu_read_unlock();

  // Allocate memory for next pointer.
  new_pos = kmalloc(sizeof(int), GFP_KERNEL);
  if (!new_pos) {
    err = -ENOMEM;
		goto cleanup;
	}
  *new_pos = last_pos;
  // Wait for readers to stop reading.
  synchronize_rcu();
  // Free old buffer.
  kfree(msg_pos);
  // Save new pointer.
  rcu_assign_pointer(msg_pos, new_pos);

	// 2. Send the text
	err = copy_to_user(user_buf, local_buf, length_to_copy);
	if (err < 0)
		goto cleanup;

	err = length_to_copy;

cleanup:
	kfree(local_buf);
	return err;
}

const struct file_operations simple_fops = {
	.read = simple_read,
};

module_init(simple_init);
module_exit(simple_exit);

