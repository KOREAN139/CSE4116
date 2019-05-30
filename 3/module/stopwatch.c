/*
 * Copyright (c) 2019 Sanggu Han
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "stopwatch.h"

static int device_open = 0;

static unsigned char *fnd_addr;

static struct st_timer {
	struct timer_list timer;
	int param;
};

struct st_timer watch_timer;

static void update_fnd(int fnd_val)
{
        outw(fnd_val, (unsigned int)fnd_addr);
}

static void stopwatch_run(unsigned long param)
{
        struct st_timer *t_data = (struct st_timer *)param;
        int i, elapsed;
        unsigned short int fnd_val = 0;

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

static ssize_t stopwatch_write(struct file *file,
                const char *buf, size_t len, loff_t *off)
{
}

static int stopwatch_open(struct inode *inode,
                struct file *file)
{
        if (device_open)
                return -EBUSY;

        device_open++;

        update_fnd(0);

        try_module_get(THIS_MODULE);
        return SUCCESS;
}

static int stopwatch_release(struct inode *inode,
                struct file *file)
{
        device_open--;

        module_put(THIS_MODULE);
        return SUCCESS;
}

struct file_operations fops = {
        .write = stopwatch_write,
        .open = stopwatch_open,
        .release = stopwatch_release,
};

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
        return 0;
}

void cleanup_module()
{
        iounmap(fnd_addr);
        del_timer_sync(&watch_timer.timer);
        unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sanggu Han");
