#include "broken_module.h"

struct proc_dir_entry *proc_entry = 0;
char *mybuf=NULL;
int mybuf_size=100;

int fill_buffer(char* buf, int buf_size) {
    sprintf(mybuf, "I've created a buffer of size: %d\n", buf_size);
    return strlen(mybuf);
}

int broken_init(void) {
    int result;

    /* Rejestracja pliku w systemie proc */
    proc_entry = proc_create("broken", 0, NULL, &broken_proc_fops);

    /* Rejestracja urzadzenia */
    result = register_chrdev(broken_major, "broken", &broken_fops);
    if (result < 0) {
        printk(KERN_WARNING "Cannot register the /dev/broken device with major number: %d\n", broken_major);
        return result;
    }

    printk(KERN_WARNING "The BROKEN module has been inserted.\n");
    return 0;
}

void broken_exit(void) {
    /* Odrejestrowanie urzadzenia */
    unregister_chrdev(broken_major, "broken");
    if (proc_entry) {
        proc_remove(proc_entry);
    }

    printk(KERN_WARNING "The BROKEN module has been removed\n");
}

int broken_open(struct inode *inode, struct file *filp) {
    return 0;
}

int broken_release(struct inode *inode, struct file *filp) {
    return 0;
}

ssize_t broken_read(struct file *filp, char *user_buf, size_t count, loff_t *f_pos) {
   char* buf = NULL;
   int buf_size = 100;
   int len, err;

    buf = kmalloc(buf_size, GFP_KERNEL);
    if (buf == 0) {
       broken_exit();
       return -ENOMEM;
    } else {
       fill_buffer(buf,buf_size);
    }
   
    len = strlen(buf);
    err = copy_to_user(user_buf, buf, len);
    kfree(buf);
    
    read_count++;


    if (!err && *f_pos == 0) {
      *f_pos += len;
      return len;
    } else {
      return 0;
    }
}

ssize_t broken_write(struct file *filp, const char *user_buf, size_t count, loff_t *f_pos) {
    return 1;
}

ssize_t broken_read_proc(struct file *filp, char *user_buf, size_t count, loff_t *f_pos) {
    char buf[100] = {0};
    int length;
    int error;

    length = snprintf(buf, 100, "BROKEN. Read calls: %d, Write calls: %d\n", read_count, write_count);
    if (count >= length) {
        count = length;
    }
    error = copy_to_user(user_buf, buf, count);	
    if (error) {
	return error;
    }
    return count;
}
