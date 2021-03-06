/*
 * Copyright (c) 2019 Sanggu Han
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "huins.h"

#define UNIT_TIME (HZ / 10)

static int device_open = 0;
const char *student_num = "20151623";
static const int num_len = 8;
const char *student_name = "Sanggu Han";
static const int name_len = 10;

static unsigned char *dot_addr;
static unsigned char *fnd_addr;
static unsigned char *led_addr;
static unsigned char *lcd_addr;

static struct st_huins_timer {
	struct timer_list timer;
	int cnt;
	int param;
};

struct st_huins_timer huins_timer;

static void huins_control_device(int pos, int val, int cnt)
{
        int i, indent;
        unsigned short int dot_val;
        unsigned short int fnd_val = val << (pos * 4);
        unsigned short led_val = 1 << (8 - val);
        unsigned short int lcd_val;

	char line_buf[16];

        for (i = 0; i < 10; i++) {
                dot_val = dot_number[val - 1][i] & 0x7f;
                outw(dot_val, (unsigned int)dot_addr + i * 2);
        }

        outw(fnd_val, (unsigned int)fnd_addr);

        outw(led_val, (unsigned int)led_addr);

	memset(line_buf, ' ', 16);
        indent = 8 - ABS((8 - cnt % 16));
	memcpy(line_buf + indent, student_num, num_len);
        for (i = 0; i < 16; i += 2) {
		lcd_val = (line_buf[i] << 8) | line_buf[i + 1];
                outw(lcd_val, (unsigned int)lcd_addr + i);
	}

	memset(line_buf, ' ', 16);
        indent = 6 - ABS((6 - cnt % 12));
	memcpy(line_buf + indent, student_name, name_len);
        for (i = 0; i < 16; i += 2) {
		lcd_val = (line_buf[i] << 8) | line_buf[i + 1];
                outw(lcd_val, (unsigned int)lcd_addr + i + 16);
        }
}

static void huins_clear_device(void)
{
	int i;
	unsigned short space = (' ' << 8) | ' ';
        for (i = 0; i < 10; i++)
                outw(0, (unsigned int)dot_addr + i * 2);
        outw(0, (unsigned int)fnd_addr);
        outw(0, (unsigned int)led_addr);
        for (i = 0; i < 32; i += 2)
                outw(space, (unsigned int)lcd_addr + i);
}

static void huins_run(unsigned long param)
{
	struct st_huins_timer *t_data = (struct st_huins_timer *)param;
       int op = t_data->param;
       int cnt = t_data->cnt;
       int pos = POSITION(op);
       int val = VALUE(op);
       int gap = INTERVAL(op);
       int lap = LAPS(op);

       if (!lap) {
               huins_clear_device();
               return;
       }
       lap -= 1;

       huins_control_device(pos, val, cnt);

       if (cnt % 8 == 7)
               pos = (pos - 1 + 4) % 4;
       val = val - 8 ? val + 1 : 1;

       t_data->param = CONSTRUCT_PARAM(pos, val, gap, lap);
       t_data->cnt += 1;

       huins_timer.timer.function = huins_run;
       huins_timer.timer.data = (unsigned long)&huins_timer;
       huins_timer.timer.expires = get_jiffies_64() + (UNIT_TIME * gap);

       add_timer(&huins_timer.timer);
}

static int huins_open(struct inode *inode,
                struct file *file)
{
        if (device_open)
                return -EBUSY;

        device_open++;

        huins_clear_device();
        
        try_module_get(THIS_MODULE);
        return SUCCESS;
}

static int huins_release(struct inode *inode,
                struct file *file)
{
        device_open--;

        module_put(THIS_MODULE);
        return SUCCESS;
}

static long huins_ioctl(struct file *file,
                unsigned int ioctl_num,
                unsigned long ioctl_param)
{
	int op;
        switch (ioctl_num) {
        case IOCTL_RUN_DEVICE:
                copy_from_user(&op,
                                (void __user *)ioctl_param, sizeof(op));
		huins_timer.cnt = 0;
		huins_timer.param = op;
                huins_run((unsigned long)&huins_timer);
                break;
        }

        return SUCCESS;
}

struct file_operations fops = {
        .unlocked_ioctl = huins_ioctl,
        .open = huins_open,
        .release = huins_release,
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
	led_addr = ioremap(LED_ADDRESS, 0x1);
	lcd_addr = ioremap(LCD_ADDRESS, 0x32);
	dot_addr = ioremap(DOT_ADDRESS, 0x10);
        init_timer(&huins_timer.timer);
        return 0;
}

void cleanup_module()
{
	iounmap(fnd_addr);
	iounmap(led_addr);
	iounmap(lcd_addr);
	iounmap(dot_addr);
        del_timer_sync(&huins_timer.timer);
        unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sanggu Han");
