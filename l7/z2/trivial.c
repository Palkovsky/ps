#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dawid Macek");

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG(fmt, ...) printk(KERN_INFO "[%s] " fmt "\n", __FILENAME__, ##__VA_ARGS__)
#define ERR(fmt, ...) printk(KERN_ERR "[%s] " fmt "\n", __FILENAME__, ##__VA_ARGS__)

static irqreturn_t irq_handler(int, void*);
static void tasklet_func(unsigned long);

#define IFACE_IRQ 19
#define IFACE_NAME "snooper"
#define IRQ_COOKIE (void*) irq_handler

DECLARE_TASKLET(tasklet, tasklet_func, IFACE_IRQ);

static irqreturn_t irq_handler(int irq, void *cookie)
{
	LOG("Network interface interrupt!");
  tasklet_schedule(&tasklet);
  return IRQ_NONE;
}

static void tasklet_func(unsigned long data)
{
	LOG("Hello from tasklet");
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

  return result;
}

static void __exit trivial_exit(void)
{
  LOG("Exit");
	free_irq(IFACE_IRQ, IRQ_COOKIE);
  tasklet_kill(&tasklet);
}

module_init(trivial_init);
module_exit(trivial_exit);
