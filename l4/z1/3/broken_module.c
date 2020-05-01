#include "broken_module.h"

struct proc_dir_entry *proc_entry = 0;
char* buf1;
char* buf2;
int buf1_size=2048;
int buf2_size=2048;


long suma=0;

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

    buf1 = (char*) kmalloc( buf1_size, GFP_KERNEL);
    buf2 = (char*) kmalloc( buf2_size, GFP_KERNEL);

    if (!buf1 || !buf2) {
        result = -ENOMEM;
        broken_exit();
        return result;
    } else {
        memset((void*) buf1, 0,  buf1_size);
	memset((void*) buf2, 0,  buf2_size);
        printk(KERN_WARNING "The BROKEN module has been inserted.\n");
        return 0;
    }
}

void broken_exit(void) {
    /* Odrejestrowanie urzadzenia */
    unregister_chrdev(broken_major, "broken");
    if (proc_entry) {
        proc_remove(proc_entry);
    }

    /* Zwolnienie bufora */
    if (buf1)
	kfree(buf1);
    if (buf2)
	kfree(buf2);

    printk(KERN_WARNING "The BROKEN module has been removed\n");
}

int broken_open(struct inode *inode, struct file *filp) {
    return 0;
}

int broken_release(struct inode *inode, struct file *filp) {
    return 0;
}

ssize_t broken_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    int buf2_count=0;
    int error=0;
    
    snprintf(buf2, buf1_size, "Process name: %s\n", buf1);
    buf2_count = strlen(buf2);
    
    error = copy_to_user(buf, buf2,buf2_count);

    read_count++;

    if (*f_pos == 0) {
        *f_pos += buf2_count;
        return buf2_count;
    } else {
        return 0;
    }
}

void fill_buffer_with_process_name(long pid) {
 struct pid *selected_pid = find_get_pid(pid);
 struct task_struct *selected_proc = pid_task(selected_pid, PIDTYPE_PID);

 if (selected_proc != NULL) {
      strcpy(buf1,(char*) selected_proc->pid);
 } else {
      sprintf(buf1,"The process with PID: %ld cannot be found",pid);
 }
}

ssize_t broken_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    int error;

    long pid=0;

    int copy_size=count;
    if (count > buf1_size)
  	copy_size = buf1_size-1;

    error = copy_from_user(buf1,buf,copy_size);
    buf1[copy_size]=0;


    // Sprobujmy sparsowac liczbe...
    pid = simple_strtol(buf1,(char**)buf1+copy_size,10);

    if (pid < 1) {
	printk(KERN_WARNING "Invalid PID number\n");
    } else {
	fill_buffer_with_process_name(pid);
    }

    write_count++;
    return copy_size;
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

