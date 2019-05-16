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
static const char *student_num = "20151623";
static const int num_len = 8;
static const char *student_name = "Sanggu Han";
static const int name_len = 10;

static unsigned char *dot_addr;
static unsigned char *fnd_addr;
static unsigned char *led_addr;
static unsigned char *lcd_addr;

static struct timer_list huins_timer;

static void huins_control_device(int pos, int val)
{
        int i, indent;
        unsigned short int dot_val;
        unsigned short int fnd_val = val << (pos * 4);
        unsigned short led_val = 1 << (8 - val);
        unsigned short int lcd_val;

        for (i = 0; i < 10; i++) {
                dot_val = dot_number[val - 1] & 0x7f;
                outw(dot_val, (unsigned int)dot_addr + i * 2);
        }

        outw(fnd_val, (unsigned int)fnd_addr);

        outw(led_val, (unsigned int)led_addr);

        indent = 8 - ABS((8 - cnt % 16));
        for (i = 0; i < num_len; i += 2) {
                lcd_val = ((student_num[i] & 0xFF) << 8)
                        | (student_num[i + 1] & 0xFF);
                outw(lcd_val, (unsigned int)lcd_addr + indent + i);
        }

        indent = 6 - ABS((6 - cnt % 12));
        for (i = 0; i < name_len; i += 2) {
                lcd_val = ((student_name[i] & 0xFF) << 8)
                        | (student_name[i + 1] & 0xFF);
                outw(lcd_val, (unsigned int)lcd_addr + 16 + indent + i);
        }
}

static void huins_clear_device()
{
        for (i = 0; i < 10; i++)
                outw(0, (unsigned int)dot_addr + i * 2);
        outw(0, (unsigned int)fnd_addr);
        outw(0, (unsigned int)led_addr);
        for (i = 0; i < 32; i += 2)
                outw(0, (unsigned int)lcd_addr + i);
}

static void huins_run(unsigned long param)
{
       int op = *(int *)param;
       int pos = POSITION(op);
       int val = VALUE(op);
       int gap = INTERVAL(op);
       int lap = LAPS(op);

       if (!lap) {
               huins_clear_device();
               return;
       }
       lap -= 1;

       huins_control_device(pos, val);

       if (cnt && !(cnt % 8))
               pos = (pos - 1 + 4) % 4;
       val = val - 8 ? val + 1 : 1;

       op = CONSTRUCT_PARAM(pos, val, gap, lap);

       cnt += 1;

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
