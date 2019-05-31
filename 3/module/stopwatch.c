/*
 * Copyright (c) 2019 Sanggu Han
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/gpio.h>
#include "stopwatch.h"

static int device_open = 0;
static unsigned long paused_at;
static unsigned char *fnd_addr;
DECLARE_WAIT_QUEUE_HEAD(wait_queue);

static struct st_timer {
	struct timer_list timer;
	int param;
};

struct st_timer watch_timer;
struct st_timer term_timer;

/*
 * term_run - terminates stopwatch & wakes user program up
 * @param: not used in this function
 */
static void term_run(unsigned long param)
{
        update_fnd(0);
        del_timer(&watch_timer.timer);
        __wake_up(&wait_queue);
}

/*
 * update_fnd - update fnd status
 * @fnd_val: 4 digits which will be displayed on fnd
 */
static void update_fnd(int fnd_val)
{
        outw(fnd_val, (unsigned int)fnd_addr);
}

/*
 * stopwatch_run - increase elapsed time & update fnd every second
 * @param: address of timer (watch_timer)
 */
static void stopwatch_run(unsigned long param)
{
        struct st_timer *t_data = (struct st_timer *)param;
        int i, elapsed;
        unsigned short int fnd_val = 0;

        /* @elapsed: 4-digit, HHMM format */
        elapsed = t_data->param;
        for (i = 0; i < 4; i++) {
                fnd_val |= (elapsed % 10) << (4 * i);
                elapsed /= 10;
        }
        update_fnd(fnd_val);

        elapsed = t_data->param + 1;
        if (elapsed % 100 == 60)
                elapsed = elapsed / 100 * 100 + 100;
        t_data->param = elapsed;

        watch_timer.timer.function = stopwatch_run;
        watch_timer.timer.data = (unsigned long)&watch_timer;
        watch_timer.timer.expires = get_jiffies_64() + HZ;

        add_timer(&watch_timer.timer);
}

/*
 * stopwatch_write - sleeps user program using wait_queue
 * @file, @buf, @len, @off: not used in this function
 */
static ssize_t stopwatch_write(struct file *file,
                const char *buf, size_t len, loff_t *off)
{
        paused_at = 0;

        interruptible_sleep_on(wait_queue);

        return SUCCESS;
}

/*
 * home_handler - interrupt handler for HOME button
 *                starts stopwatch if it's inactive
 * @irq, @dev_id, @regs: not used in this function
 */
irqreturn_t home_handler(int irq, void *dev_id, struct ptr_regs *regs)
{
        if (!timer_pending(&watch_timer.timer)) {
                paused_at = 0;
                watch_timer.param = 0;
                stopwatch_run((unsigned long)&watch_timer);
        }

        return IRQ_HANDLED;
}

/*
 * back_handler - interrupt handler for BACK button
 *                pauses stopwatch if it's active
 *                resumes if it's paused
 * @irq, @dev_id, @regs: not used in this function
 */
irqreturn_t back_handler(int irq, void *dev_id, struct ptr_regs *regs)
{
        if (timer_pending(&watch_timer.timer)) {
                paused_at = watch_timer.timer.expires - get_jiffies_64();
                del_timer(&watch_timer.timer);
        } else {
                watch_timer.timer.expires = get_jiffies_64() + HZ - paused_at;
                add_timer(&watch_timer.timer);
        }

        return IRQ_HANDLED;
}

/*
 * volume_up_handler - interrupt handler for VOL+ button
 *                     resets stopwatch to initial state
 * @irq, @dev_id, @regs: not used in this function
 */
irqreturn_t volume_up_handler(int irq, void *dev_id, struct ptr_regs *regs)
{
        paused_at = 0;
        watch_timer.param = 0;
        update_fnd(0);
        if (timer_pending(&watch_timer.timer)) {
                mod_timer(&watch_timer.timer, get_jiffies_64() + HZ);
        }

        return IRQ_HANDLED;
}

/*
 * volume_down_handler - interrupt handler for VOL- button
 *                       add timer for termination when button is pressed
 *                       delete timer if any when button is released
 * @irq, @dev_id, @regs: not used in this function
 */
irqreturn_t volume_down_handler(int irq, void *dev_id, struct ptr_regs *regs)
{
        if (gpio_get_value(IMX_GPIO_NR(5, 14))) {
                if (timer_pending(&term_timer.timer)) {
                        mod_timer(&term_timer.timer,
                                        get_jiffies_64() + 3 * HZ);
                } else {
                        term_timer.timer.function= term_run();
                        term_timer.timer.expires = get_jiffies_64() + 3 * HZ;
                        add_timer(&term_timer.timer);
                }
        } else {
                if (timer_pending(&term_timer.timer))
                        del_timer(&term_timer.timer);
        }

        return IRQ_HANDLED;
}

/*
 * stopwatch_open - register interrupts & opens module 
 * @inode, @file: not used in this function
 */
static int stopwatch_open(struct inode *inode,
                struct file *file)
{
        if (device_open)
                return -EBUSY;

        int irq, ret;
        unsigned int irq_flag = IRQF_TRIGGER_RISING;

        update_fnd(0);

        gpio_direction_input(IMX_GPIO_NR(1, 11));
        irq = gpio_to_irq(IMX_GPIO_NR(1, 11));
        ret = request_irq(irq, home_handler,
                        irq_flag, "HOME_BTN", NULL);
        if (ret)
                printk("ERROR: Cannot request IRQ %d\n - code %d\n", irq, ret);

        gpio_direction_input(IMX_GPIO_NR(1, 12));
        irq = gpio_to_irq(IMX_GPIO_NR(1, 12));
        ret = request_irq(irq, back_handler,
                        irq_flag, "BACK_BTN", NULL);
        if (ret)
                printk("ERROR: Cannot request IRQ %d\n - code %d\n", irq, ret);

        gpio_direction_input(IMX_GPIO_NR(2, 15));
        irq = gpio_to_irq(IMX_GPIO_NR(2, 15));
        ret = request_irq(irq, volume_up_handler,
                        irq_flag, "VOLUP_BTN", NULL);
        if (ret)
                printk("ERROR: Cannot request IRQ %d\n - code %d\n", irq, ret);

        gpio_direction_input(IMX_GPIO_NR(5, 14));
        irq = gpio_to_irq(IMX_GPIO_NR(5, 14));
        irq_flag |= IRQF_TRIGGER_FALLING;
        ret = request_irq(irq, volume_down_handler,
                        irq_flag, "VOLDOWN_BTN", NULL);
        if (ret)
                printk("ERROR: Cannot request IRQ %d\n - code %d\n", irq, ret);

        device_open++;

        try_module_get(THIS_MODULE);
        return SUCCESS;
}

/*
 * stopwatch_release - free registered interrupts & release module
 * @inode, @file: not used in this function
 */
static int stopwatch_release(struct inode *inode,
                struct file *file)
{
        device_open--;

        free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11), NULL);
        free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12), NULL);
        free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15), NULL);
        free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14), NULL);

        module_put(THIS_MODULE);
        return SUCCESS;
}

struct file_operations fops = {
        .write = stopwatch_write,
        .open = stopwatch_open,
        .release = stopwatch_release,
};

/*
 * init_module - register this device driver to kernel
 */
int init_module()
{
        int ret;
        ret = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);
        if (ret < 0) {
                printk(KERN_ALERT "%s failed with %d\n",
                                "Sorry, registering the character device ",
                                ret);
                return ret;
        }
        fnd_addr = ioremap(FND_ADDRESS, 0x4);
        init_timer(&watch_timer.timer);
        init_timer(&term_timer.timer);
        return 0;
}

/*
 * cleanup_module - unregister this device driver from kernel
 */
void cleanup_module()
{
        iounmap(fnd_addr);
        del_timer_sync(&watch_timer.timer);
        unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sanggu Han");
