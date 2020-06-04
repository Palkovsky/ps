#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#define SYS_MYSYSCALL   436
#define SYS_TOPUSER     440
#define SYS_TIMEUSER    441
#define SYS_KERNELPS    442
#define SYS_FREEBLOCKSS 443
#define SYS_PIDTONAME   444

#define LOG(fmt, ...) printf("[%s] " fmt "\n", __func__, ##__VA_ARGS__)

static long result;

static long mysyscall(void) {
  if((result = syscall(SYS_MYSYSCALL))) {
    LOG("FAILED");
    return result;
  }
  LOG("OK");
  return 0;
}

static long topuser(void) {
  uid_t uid;
  if((result = syscall(SYS_TOPUSER, &uid))) {
    LOG("FAILED");
    return result;
  }
  LOG("OK. UID: %d", uid);
  return 0;
}

static long timeuser(void) {
  uid_t uid;
  if((result = syscall(SYS_TIMEUSER, &uid))) {
    LOG("FAILED");
    return result;
  }
  LOG("OK. UID: %d", uid);
  return 0;
}

static long kernelps(void) {
  size_t sz;
  char buff[1<<20];

  // buff=NULL case
  if((result = syscall(SYS_KERNELPS, &sz, NULL))) {
    LOG("buff=NULL FAILED with %d", result);
  } else {
    LOG("buff=NULL OK. Size: %ld", sz);
  }

  if((result = syscall(SYS_KERNELPS, &sz, buff))) {
    LOG("buff!=NULL FAILED with %d", result);
  } else {
    // This prints in reverse.
    while(sz--) {
      LOG("%s", buff+sz*16);
    }
    LOG("buff!=NULL OK. Entries: %ld", sz);
  }

  return 0;
}

static long freeblocks(void) {
  char *filpath;
  uint64_t avail;

  filpath = "/dev/sda";
  if((result = syscall(SYS_FREEBLOCKSS, filpath, &avail))) {
    LOG("FAILED");
  } else {
    LOG("OK. FREE BYTES on '%s': %lld", filpath, avail);
  }

  filpath = "/sbin/init";
  if((result = syscall(SYS_FREEBLOCKSS, filpath, &avail))) {
    LOG("FAILED");
  } else {
    LOG("OK. FREE BYTES on '%s': %lld", filpath, avail);
  }

  filpath = "/sbin/initt";
  if((result = syscall(SYS_FREEBLOCKSS, filpath, &avail))) {
    LOG("OK EXPECTED ERROR: %d", result);
  } else {
    LOG("FAILED. ERROR EXPECTED");
  }

  LOG("TIP: To validate corectness use: 'df -B 1'");

  return 0;
}

static long pidtoname(void) {
  pid_t pid;
  char buff[1<<8];

  pid = 1;
  if((result = syscall(SYS_PIDTONAME, pid, buff))) {
    LOG("FAILED WITH %d", result);
  } else {
    // Should be init.
    LOG("OK. PROC NAME: %s", buff);
  }

  pid = getpid();
  if((result = syscall(SYS_PIDTONAME, pid, buff))) {
    LOG("FAILED WITH %d", result);
  } else {
    // Should be current process.
    LOG("OK. PROC NAME: %s", buff);
  }

  pid = -1;
  if((result = syscall(SYS_PIDTONAME, pid, buff))) {
    LOG("OK EXPECTED RESULT %d", result);
  } else {
    LOG("FAILED EXPECTED ERROR");
  }

  return 0;
}

int main(int argc, char **argv) {
  mysyscall();
  LOG("-------------");
  topuser();
  LOG("-------------");
  timeuser();
  LOG("-------------");
  kernelps();
  LOG("-------------");
  freeblocks();
  LOG("-------------");
  pidtoname();
  LOG("-------------");
  LOG("TIP: Use /Shift+PageUp/ to scroll in TTY.");
  return 0;
}
