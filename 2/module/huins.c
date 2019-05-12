/*
 * Copyright (c) 2019 Sanggu Han
 */

#include<linux/kernel.h>
#include<linux/moudle.h>
#include<linux/fs.h>

#define DEVICE_NAME "dev_driver"
#define MAJOR_NUM 242

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
        return 0;
}

void cleanup_module()
{
        int ret;
        ret = unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
        if (ret < 0)
                printk(KERN_ALERT "Error: unregister_chrdev: %d\n", ret);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sanggu Han");
