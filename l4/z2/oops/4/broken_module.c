#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define BROKEN_MAJOR 899

const struct file_operations broken_fops;
const struct file_operations broken_proc_fops;

/* Zmienne globalne */
const int broken_major = 899;
int read_count;
int write_count;

int numbers_count;
struct proc_dir_entry *proc_entry;

/* Fills the buffer with numeric character stats */
int fill_buffer(char *buf)
{
	sprintf(buf, "I've recently read %d numeric characters\n",
		numbers_count);
	return strlen(buf);
}

/* Counts number of digits in provided string */
int count_numbers(char *str)
{
	int numbers = 0;
	char *ptr = 0;

	while (*ptr != 0) {
		ptr++;
		if (isdigit(*ptr))
			numbers++;
	}

	return numbers;
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

	// Initialize memory
	mybuf = kmalloc(mybuf_size, GFP_KERNEL);
	if (!mybuf) {
		return -ENOMEM;
	}

	// Fill up the buffer
	fill_buffer(mybuf);
	len = strlen(mybuf);

	// Copy it to user space
	err = copy_to_user(user_buf, mybuf, len);

	// Free up the memory
	kfree(mybuf);

	read_count++;

	if (!err && *f_pos == 0) {
		*f_pos += len;
		return len;
	} else {
		return 0;
	}
}

ssize_t broken_write(struct file *filp, const char *user_buf, size_t count,
	loff_t *f_pos)
{
	char *mybuf = NULL;
	int mybuf_size = 100;
	int real_count = count;
	int err;

	// Initialize the memory
	kmalloc(mybuf_size, GFP_KERNEL);

	// Take the max(mybuf_size, count)
	if (real_count > mybuf_size)
		real_count = mybuf_size;

	// Copy the buffer from user space
	err = copy_from_user(mybuf, user_buf, real_count);
	mybuf[mybuf_size] = 0;

	if (!err && real_count > 0) {
		// Count number of digits in buffer
		numbers_count = count_numbers(mybuf);
	} else {
		printk(KERN_WARNING "BROKEN: error occured in write function");
	}

	// Free the memory
	if (mybuf != NULL)
		kfree(mybuf);

	write_count++;
	return real_count;
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

