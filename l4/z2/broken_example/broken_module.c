#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>

MODULE_LICENSE("GPL");

#define BROKEN_MAJOR 899

const struct file_operations broken_fops;

static int __init broken_init(void)
{
	int result = 0;

	/* Register a device with the given major number */
	result = register_chrdev(BROKEN_MAJOR, "broken", &broken_fops);
	if (result < 0) {
		printk(KERN_WARNING
			"Cannot register the /dev/broken device with major number: %d\n",
			BROKEN_MAJOR);
		goto err;
	}
	printk(KERN_INFO "The broken module has been inserted.\n");
	return result;

err:
	unregister_chrdev(BROKEN_MAJOR, "broken");
	return result;
}

static void __exit broken_exit(void)
{
	unregister_chrdev(BROKEN_MAJOR, "broken");
	printk(KERN_WARNING "The broken module has been removed\n");
}

ssize_t broken_read(struct file *filp, char __user *user_buf,
	size_t count, loff_t *f_pos)
{
	*(int *)0 = 0;
	return 0;
}

const struct file_operations broken_fops = {
	.read = broken_read,
};

module_init(broken_init);
module_exit(broken_exit);
