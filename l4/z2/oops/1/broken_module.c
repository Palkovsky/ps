#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define BROKEN_MAJOR 899

const struct file_operations broken_fops;
const struct file_operations broken_proc_fops;

/* Zmienne globalne */
const int broken_major = 899;
int read_count;
int write_count;

struct proc_dir_entry *proc_entry;

int fill_buffer(char *buf, int buf_size)
{
	sprintf(buf, "I've created a buffer of size: %d\n", buf_size);
	return strlen(buf);
}

int broken_init(void)
{
	int result;

	/* Rejestracja pliku w systemie proc */
	proc_entry = proc_create("broken", 0000, NULL, &broken_proc_fops);

	/* Rejestracja urzadzenia */
	result = register_chrdev(broken_major, "broken", &broken_fops);
	if (result < 0) {
		printk(KERN_WARNING
			"Cannot register the /dev/broken device with major number: %d\n",
			broken_major);
		return result;
	}

	printk(KERN_WARNING "The BROKEN module has been inserted.\n");
	return 0;
}


void broken_exit(void)
{
	/* Odrejestrowanie urzadzenia */
	unregister_chrdev(broken_major, "broken");
	if (proc_entry)
		proc_remove(proc_entry);

	printk(KERN_WARNING "The BROKEN module has been removed\n");
}

ssize_t broken_read(struct file *filp, char *user_buf, size_t count,
	loff_t *f_pos)
{
	char *mybuf = NULL;
	int mybuf_size = 100;
	int len, err;

	mybuf = kmalloc(mybuf_size, GFP_KERNEL);
	if (!mybuf) {
		return -ENOMEM;
	}

	fill_buffer(mybuf, mybuf_size);

	len = strlen(mybuf);
	err = copy_to_user(user_buf, mybuf, len);
	kfree(user_buf);

	read_count++;

	if (!err && *f_pos == 0) {
		*f_pos += len;
		return len;
	}
	return 0;
}

ssize_t broken_write(struct file *filp, const char *user_buf, size_t count,
	loff_t *f_pos)
{
	return 1;
}

ssize_t broken_read_proc(struct file *filp, char *user_buf, size_t count,
	loff_t *f_pos)
{
	char buf[100] = {0};
	int length;
	int error;

	length = snprintf(buf, 100, "BROKEN. Reads: %d, Writes: %d\n",
		read_count, write_count);
	if (count >= length)
		count = length;

	error = copy_to_user(user_buf, buf, count);
	if (error)
		return error;

	return count;
}

const struct file_operations broken_fops = {
	.read = broken_read,
	.write = broken_write,
};

const struct file_operations broken_proc_fops = {
	.read = broken_read_proc
};

module_init(broken_init);
module_exit(broken_exit);

