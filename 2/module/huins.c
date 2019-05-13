/*
 * Copyright (c) 2019 Sanggu Han
 */

#include<linux/kernel.h>
#include<linux/moudle.h>
#include<linux/fs.h>
#include<asm/io.h>
#include<huins.h>

static unsigned char *dot_addr;
static unsigned char *fnd_addr;
static unsigned char *led_addr;
static unsigned char *lcd_addr;

static int huins_open(struct inode *inode,
                struct file *file)
{
}

static int huins_release(struct inode *inode,
                struct file *file)
{
}

static int huins_ioctl(struct inode *inode,
                struct file *file,
                unsigned int ioctl_num,
                unsigned long ioctl_param)
{
}

struct file_operations fops = {
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
        return 0;
}

void cleanup_module()
{
        int ret;
	iounmap(fnd_addr);
	iounmap(led_addr);
	iounmap(lcd_addr);
	iounmap(dot_addr);
        ret = unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
        if (ret < 0)
                printk(KERN_ALERT "Error: unregister_chrdev: %d\n", ret);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sanggu Han");
