#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/limits.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/statfs.h>

typedef int (*statfs_f) (struct dentry *, struct kstatfs *);

SYSCALL_DEFINE2(freeblocks, char __user *, path, u64 __user*, freeblks)
{
  long err = 0;
  char *pathbuff;
  struct path filepath;
  struct super_block *superblock;
  statfs_f statfs_func;
  struct kstatfs statfs;
  u64 bytes_free;

  pathbuff = kmalloc(PATH_MAX, GFP_KERNEL);
  if(!pathbuff) {
    printk (KERN_EMERG "[sys_freeblocks]: Unable to allocate memory for path buffer.\n");
    err = -ENOMEM;
    goto ret;
  }

  // Copy path to kernel space buffer.
  if(copy_from_user(pathbuff, path, PATH_MAX)) {
    printk (KERN_EMERG "[sys_freeblocks]: Unable to copy path to kernel-spacer buffer.\n");
    err = -EFAULT;
    goto cleanup;
  }

  // Try to resolve path.
  if(kern_path(pathbuff, LOOKUP_FOLLOW, &filepath)) {
    printk (KERN_EMERG "[sys_freeblocks]: Unable to resolve path '%s'.\n", pathbuff);
    err = -ENOENT;
    goto cleanup;
  }

  // Get superblock of the mounted filesystem and statfs function.
  superblock = filepath.mnt->mnt_sb;
  statfs_func = superblock->s_op->statfs;

  if(!statfs_func) {
    printk(KERN_EMERG "[sys_freeblocks] Unable to acquire statfs function.");
    err = -EFAULT;
    goto cleanup;
  }

  // Call statfs function
  if((err = statfs_func(superblock->s_root, &statfs))) {
    printk(KERN_EMERG "[sys_freeblocks] Statfs error.");
    goto cleanup;
  }

  // Copy free blocks number to user-space.
  bytes_free = statfs.f_bavail*statfs.f_bsize;
  err = copy_to_user(freeblks, &bytes_free, sizeof(u64));
  if(err) {
    printk(KERN_EMERG "[sys_freeblocks] Error copying free blocks number to the user-space.");
  }

 cleanup:
  kfree(pathbuff);
 ret:
  return err;
}
