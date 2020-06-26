#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dawid Macek");

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG(fmt, ...) printk(KERN_INFO "[%s] " fmt "\n", __FILENAME__, ##__VA_ARGS__)
#define ERR(fmt, ...) printk(KERN_ERR "[%s] " fmt "\n", __FILENAME__, ##__VA_ARGS__)

// IRQ handler
static irqreturn_t irq_handler(int, void*);
#define IFACE_IRQ 19
#define IFACE_NAME "snooper"
#define IRQ_COOKIE (void*) irq_handler

// Tasklet
static void tasklet_callback(unsigned long);
DECLARE_TASKLET(tasklet, tasklet_callback, IFACE_IRQ);

// Timer
#define TIMER_TIMEOUT 300
static void timer_callback(struct timer_list *);
static struct timer_list timer;

static irqreturn_t irq_handler(int irq, void *cookie)
{
	LOG("Network interface interrupt!");
  tasklet_schedule(&tasklet);
  return IRQ_NONE;
}

static void tasklet_callback(unsigned long data)
{
  if (timer_pending(&timer)) {
    LOG("TASKLET - Timer pending. Won't reschedule.");
  } else {
    LOG("TASKLET - Timer scheduled for %dms", TIMER_TIMEOUT);
    mod_timer(&timer, jiffies + msecs_to_jiffies(TIMER_TIMEOUT));
  }
}

static void timer_callback(struct timer_list *data)
{
	LOG("Hello from timer");
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

  timer_setup(&timer, timer_callback, 0);

  return result;
}

static void __exit trivial_exit(void)
{
  LOG("Exit");
	free_irq(IFACE_IRQ, IRQ_COOKIE);
  tasklet_kill(&tasklet);
  del_timer(&timer);
}

module_init(trivial_init);
module_exit(trivial_exit);
