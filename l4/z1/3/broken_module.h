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

int broken_open(struct inode *inode, struct file *filp);
int broken_release(struct inode *inode, struct file *filp);
ssize_t broken_read(struct file *filp, char *user_buf, size_t count, loff_t *f_pos);
ssize_t broken_write(struct file *filp, const char *user_buf, size_t count, loff_t *f_pos);
ssize_t broken_read_proc(struct file *filep, char *user_buf, size_t count, loff_t *f_pos);
void broken_exit(void);
int broken_init(void);

/* Struktura ktora opisuje funkcje uzywane przy dostepie do /dev/broken */
struct file_operations broken_fops = {
    read: broken_read,
    write: broken_write,
    open: broken_open,
    release: broken_release
};

struct file_operations broken_proc_fops = {
   read: broken_read_proc
};

/* Deklaracje standardowych funkcje inicjalizacji i wyjscia modulu */
module_init(broken_init);
module_exit(broken_exit);

/* Zmienne globalne */
int broken_major = 500;
int read_count = 0;
int write_count = 0;
char *buf;
char *strBuf;
