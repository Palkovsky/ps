#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
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

char *buf;
char *strBuf;
char *buf1;
char *buf2;
int buf1_size = 2048;
int buf2_size = 2048;
long suma;

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

	buf1 = kmalloc(buf1_size, GFP_KERNEL);
	buf2 = kmalloc(buf2_size, GFP_KERNEL);

	if (!buf1 || !buf2) {
		result = -ENOMEM;
		kfree(buf1);
		kfree(buf2);
		return result;
	}

	memset(buf1, 0,  buf1_size);
	memset(buf2, 0,  buf2_size);
	printk(KERN_WARNING "The BROKEN module has been inserted.\n");
	return 0;
}

void broken_exit(void)
{
	/* Odrejestrowanie urzadzenia */
	unregister_chrdev(broken_major, "broken");
	if (proc_entry)
		proc_remove(proc_entry);

	/* Zwolnienie bufora */
	kfree(buf1);
	kfree(buf2);

	printk(KERN_WARNING "The BROKEN module has been removed\n");
}

ssize_t broken_read(struct file *filp, char *buf, size_t count,
	loff_t *f_pos)
{
	int buf2_count = 0;
	int error = 0;

	snprintf(buf2, buf1_size, "Process name: %s\n", buf1);
	buf2_count = strlen(buf2);

	error = copy_to_user(buf, buf2, buf2_count);

	read_count++;

	if (*f_pos == 0) {
		*f_pos += buf2_count;
		return buf2_count;
	}
	return 0;
}

void fill_buffer_with_process_name(long pid)
{
	struct pid *selected_pid = find_get_pid(pid);
	struct task_struct *selected_proc = pid_task(selected_pid, PIDTYPE_PID);

	if (selected_proc != NULL)
		strcpy(buf1, (char *) selected_proc->pid);
	else
		sprintf(buf1, "The process with PID: %ld cannot be found", pid);
}

ssize_t broken_write(struct file *filp, const char *buf, size_t count,
		     loff_t *f_pos)
{
	int error = 0;
	long pid = 0;
	int copy_size = count;

	if (count > buf1_size)
		copy_size = buf1_size - 1;

	error = copy_from_user(buf1, buf, copy_size);
	buf1[copy_size] = 0;

	pid = simple_strtol(buf1, buf1 + copy_size, 10);

	if (pid < 1)
		printk(KERN_WARNING "Invalid PID number\n");
	else
		fill_buffer_with_process_name(pid);

	write_count++;
	return copy_size;
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


