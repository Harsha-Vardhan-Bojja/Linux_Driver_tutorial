#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/of.h>

#define WR_VALUE _IOW('a', 'a', char*)  // Write IOCTL command
#define RD_VALUE _IOR('a', 'b', char*)  // Read IOCTL command

dev_t dev = 0;
struct class *my_class;
struct device *my_device;
struct cdev my_cdev;
int *kernel_buffer;  // Kernel buffer


static int my_open(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);
static int my_close(struct inode *inode, struct file *file);
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int my_platform_remove(struct platform_device *pdev);  // Define the remove function

static struct file_operations fop = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_close,
    .unlocked_ioctl = my_ioctl,
};

static int my_open(struct inode *inode, struct file *file) {
    kernel_buffer = kmalloc(1024, GFP_KERNEL);
    if (kernel_buffer == NULL) {
        pr_err("Cannot allocate memory in kernel\n");
        return -ENOMEM;
    }
    pr_info("Module successfully opened\n");
    return 0;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    return count;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    return count;
}

static int my_close(struct inode *inode, struct file *file) {
    kfree(kernel_buffer);
    kernel_buffer = NULL;  // Ensure the buffer is NULL after freeing
    pr_info("Module successfully closed\n");
    return 0;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    pr_info("IOCTL has been opened for communication\n");
    int ret;
    switch (cmd) {
        case RD_VALUE:
            ret = copy_to_user((char *)arg, kernel_buffer, 1024);
            if (ret < 0) {
                pr_err("Failed to read\n");
                return ret;
            }
            pr_info("Module successfully read\n");
            return 0;

        case WR_VALUE:
            ret = copy_from_user(kernel_buffer, (char *)arg, 1024);
            if (ret < 0) {
                pr_err("Failed to write\n");
                return ret;
            }
            pr_info("Module successfully written\n");
            return 0;

        default:
            pr_info("Enter a valid option\n");
            return -EINVAL;
    }
    return 0;
}

static int my_platform_probe(struct platform_device *pdev) {
    int ret;

    // Allocate the character device region
    ret = alloc_chrdev_region(&dev, 0, 1, "my_virtual_device");
    if (ret < 0) {
        pr_err("Failed to allocate Major and Minor devices\n");
        return ret;
    }

    // Create the class
    my_class = class_create("my_virtual_device");
    if (IS_ERR(my_class)) {
        unregister_chrdev_region(dev, 1);
        pr_err("Failed to create the class\n");
        return PTR_ERR(my_class);
    }

    // Initialize the character device
    cdev_init(&my_cdev, &fop);
    ret = cdev_add(&my_cdev, dev, 1);
    if (ret < 0) {
        class_destroy(my_class);
        unregister_chrdev_region(dev, 1);
        pr_err("Cannot add the device\n");
        return ret;
    }

    // Create the device file
    my_device = device_create(my_class, NULL, dev, NULL, "my_virtual_device");
    if (IS_ERR(my_device)) {
        cdev_del(&my_cdev);
        class_destroy(my_class);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(my_device);
    }

    pr_info("Platform device probed successfully\n");

    return 0;
}

static int my_platform_remove(struct platform_device *pdev) {
    device_destroy(my_class, dev);
    cdev_del(&my_cdev);
    class_destroy(my_class);
    unregister_chrdev_region(dev, 1);
    pr_info("Platform device removed\n");
    return 0;
}

static struct of_device_id my_platform_driver_id_table = {
    .compatible = "my_platform_driver",
};


static struct platform_driver my_platform_driver = {
    .probe = my_platform_probe,
    .remove = my_platform_remove,
    .driver = {
        .name = "my_platform_driver",
        .of_match_table = &my_platform_driver_id_table,
    },
};

static struct resource my_platform_device_res = {
    .start = 0x2000,
    .end = 0x4000,
    .flags =IORESOURCE_MEM,
};

static struct platform_device my_platform_device = {
    .name = "my_platform_driver",
    .id = -1,
    .num_resources= 1,
    .resource = &my_platform_device_res,
    
};

static int __init my_init(void) {
    int ret;

    // Register the platform device
    ret = platform_device_register(&my_platform_device);
    if (ret) {
        printk(KERN_ERR "Failed to register platform device\n");
        return ret;
    }

    // Register the platform driver
    ret = platform_driver_register(&my_platform_driver);
    if (ret) {
        printk(KERN_ERR "Failed to register platform driver\n");
        platform_device_unregister(&my_platform_device);
    }
    return ret;
}

static void __exit my_exit(void) {
    platform_driver_unregister(&my_platform_driver);
    platform_device_unregister(&my_platform_device);
    printk(KERN_INFO "Platform driver and device unregistered\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bojja <balasai@gmail.com>");
MODULE_DESCRIPTION("Simple Platform Device Driver");
MODULE_VERSION("1.1");

