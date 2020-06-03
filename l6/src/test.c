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

#define LOG(fmt, ...) printf("[%s] " #fmt "\n", __func__, ##__VA_ARGS__)

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
  char buff[1<<8];
  if((result = syscall(SYS_KERNELPS, &sz, buff, 1))) {
    LOG("FAILED");
    return result;
  }
  LOG("OK");
  return 0;
}

static long freeblocks(void) {
  const static char *filpath = "/dev/sda";
  uint64_t avail_blocks;
  if((result = syscall(SYS_FREEBLOCKSS, filpath, &avail_blocks))) {
    LOG("FAILED");
    return result;
  }
  LOG("OK");
  return 0;
}

static long pidtoname(void) {
  pid_t pid = 1;
  char buff[1<<8];
  if((result = syscall(SYS_PIDTONAME, pid, buff))) {
    LOG("FAILED");
    return result;
  }
  LOG("OK");
  return 0;
}

int main() {
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
  return 0;
}
