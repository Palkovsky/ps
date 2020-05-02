#include "broken_module.h"

int numbers_count=0;
struct proc_dir_entry *proc_entry = 0;

/* Fills the buffer with numeric character stats */
int fill_buffer(char* buf) {
    sprintf(buf, "I've recently read %d numeric characters\n", numbers_count);
    return strlen(buf);
}

/* Counts number of digits in provided string */
int count_numbers(char* str) {
   int numbers = 0;
   char* ptr = str;

   while (*ptr != 0) {
     ptr++;
     if (isdigit(*ptr))
 	numbers++;
   }

   return numbers;
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
   char* mybuf = NULL;
   int mybuf_size = 100;
   int len, err;

    // Initialize memory
    mybuf = kmalloc(mybuf_size, GFP_KERNEL);
    if (!mybuf) {
       broken_exit();
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

ssize_t broken_write(struct file *filp, const char *user_buf, size_t count, loff_t *f_pos) {
   char* mybuf = NULL;
   int mybuf_size = 100;
   int real_count = count;
   int err;

    // Initialize the memory
    mybuf = kmalloc(mybuf_size+1, GFP_KERNEL);

    // Take the max(mybuf_size, count)
    if (real_count > mybuf_size)
       real_count = mybuf_size;

    // Copy the buffer from user space
    err = copy_from_user(mybuf,user_buf,real_count);
    mybuf[mybuf_size] = 0;

    if (!err && real_count > 0) {
        // Count number of digits in buffer
      numbers_count = count_numbers(mybuf);
    } else {
      printk(KERN_WARNING "BROKEN: error occured in write function");
    }

    // Free the memory
    kfree(mybuf);

    write_count++;
    return real_count;
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
