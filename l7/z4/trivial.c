#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dawid Macek");

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG(fmt, ...) printk(KERN_INFO "[%s] " fmt "\n", __FILENAME__, ##__VA_ARGS__)
#define ERR(fmt, ...) printk(KERN_ERR "[%s] " fmt "\n", __FILENAME__, ##__VA_ARGS__)
#define DELAY(msecs) (msecs_to_jiffies(msecs))

// IRQ
static irqreturn_t irq_handler(int, void*);
#define IFACE_IRQ 19
#define IFACE_NAME "snooper"
#define IRQ_COOKIE (void*) irq_handler

// Global(default) workqueue
#define WORKER_DELAY 300
static void work1_callback(struct work_struct*);
DECLARE_DELAYED_WORK(work1, work1_callback);

// Dynamic workqueue
static struct work_struct work2;
static void work2_callback(struct work_struct*);

static irqreturn_t irq_handler(int irq, void *cookie)
{
	LOG("Network interface interrupt!");
  if (delayed_work_pending(&work1)) {
    LOG("Worker pending. Won't reshedule.");
  } else {
    schedule_delayed_work(&work1, DELAY(WORKER_DELAY));
    LOG("Worker sheduled with delay of %dms", WORKER_DELAY);
  }
  return IRQ_NONE;
}

static void work1_callback(struct work_struct *work)
{
	LOG("Hello from global workqueue!");
}

static void work2_callback(struct work_struct *work)
{
	LOG("Hello from dynamic workqueue!");
}

static int __init trivial_init(void)
{
  int result = 0;

  LOG("Init");
  result = request_irq(IFACE_IRQ, irq_handler, IRQF_SHARED, IFACE_NAME, IRQ_COOKIE);
  if(result) {
    ERR("Unable to register interrupt.");
    result = -EFAULT;
  }

  INIT_WORK(&work2, work2_callback);
  schedule_work(&work2);

  return result;
}

static void __exit trivial_exit(void)
{
  LOG("Exit");
	free_irq(IFACE_IRQ, IRQ_COOKIE);
  cancel_delayed_work_sync(&work1);
  cancel_work_sync(&work2);
}

module_init(trivial_init);
module_exit(trivial_exit);
