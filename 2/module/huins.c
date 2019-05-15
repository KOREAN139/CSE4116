/*
 * Copyright (c) 2019 Sanggu Han
 */
#include <linux/kernel.h>
#include <linux/moudle.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <asm/io.h>
#include "huins.h"

#define UNIT_TIME (HZ / 10)

static int device_open = 0;
static int cnt;
static char fnd_array[4];
static const char *student_num = "20151623";
static const int num_len = 8;
static const char *student_name = "Sanggu Han";
static const int name_len = 10;

static unsigned char *dot_addr;
static unsigned char *fnd_addr;
static unsigned char *led_addr;
static unsigned char *lcd_addr;

static struct timer_list huins_timer;

static void huins_run(unsigned long param)
{
       int op = *(int *)param;
       int pos = POSITION(op);
       int val = VALUE(op);
       int gap = INTERVAL(op);
       int lap = LAPS(op);

       if (!lap)
               return;
       lap -= 1;

       // do control huins board here

       op = CONSTRUCT_PARAM(pos, val, gap, lap);

       huins_timer.function = huins_run;
       huins_timer.data = (unsigned long)&op;
       huins_timer.expires = get_jiffies_64() + (UNIT_TIME * gap);

       add_timer(&huins_timer);
}

static int huins_open(struct inode *inode,
                struct file *file)
{
        if (device_open)
                return -EBUSY;

        device_open++;

        cnt = 0;
        memset(fnd_array, 0, sizeof(fnd_array));
        
        try_module_get(THIS_MODULE);
        return SUCCESS;
}

static ssize_t huins_write(struct file *file,
                const char __user *buffer, size_t length, loff_t *offset)
{
        int param = (int)buffer;
        huins_run((unsigned long)param);
        return SUCCESS;
}

static int huins_release(struct inode *inode,
                struct file *file)
{
        device_open--;

        module_put(THIS_MODULE);
        return SUCCESS;
}

static int huins_ioctl(struct inode *inode,
                struct file *file,
                unsigned int ioctl_num,
                unsigned long ioctl_param)
{
        switch (ioctl_num) {
        case IOCTL_RUN_DEVICE:
                huins_write(file, (char *)ioctl_param, 4, 0);
                break;
        }
}

struct file_operations fops = {
        .write = huins_write,
        .ioctl = huins_ioctl,
        .open = huins_open,
        .release = huins_release,
};

static int init_module()
{
        int ret;
        ret = register_chardev(MAJOR_NUM, DEVICE_NAME, &fops);
        if (ret < 0) {
                printk(KERN_ALERT "%s failed with %d\n",
                                "Sorry, registering the character device ",
                                ret_val);
                return ret;
        }
	fnd_addr = ioremap(FND_ADDRESS, 0x4);
	led_addr = ioremap(LED_ADDRESS, 0x1);
	lcd_addr = ioremap(LCD_ADDRESS, 0x32);
	dot_addr = ioremap(DOT_ADDRESS, 0x10);
        init_timer(&huins_timer);
        return 0;
}

void cleanup_module()
{
        int ret;
	iounmap(fnd_addr);
	iounmap(led_addr);
	iounmap(lcd_addr);
	iounmap(dot_addr);
        del_timer_sync(&huins_timer);
        ret = unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
        if (ret < 0)
                printk(KERN_ALERT "Error: unregister_chrdev: %d\n", ret);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sanggu Han");
