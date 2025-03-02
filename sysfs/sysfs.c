#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define BUFFER_SIZE 100

dev_t dev;
static struct class *my_class;
static struct cdev my_cdev;
struct kobject *my_kobject;
static char sysfs_buffer[BUFFER_SIZE] = "Initial Data";

/* Function prototypes */
static int my_open(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);
static int my_close(struct inode *inode, struct file *file);

/* File operations structure */
static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .read    = my_read,
    .write   = my_write,
    .release = my_close,
};

/* Sysfs show function */
static ssize_t show_file(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%s\n", sysfs_buffer);
}

/* Sysfs store function */
static ssize_t store_file(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    if (count >= BUFFER_SIZE)  
        count = BUFFER_SIZE - 1; // Prevent buffer overflow

    strncpy(sysfs_buffer, buf, count);
    sysfs_buffer[count] = '\0'; // Ensure null termination

    return count;
}

/* Define sysfs attributes */
static struct kobj_attribute my_example1 = __ATTR(my_example1, 0664, show_file, store_file);
static struct kobj_attribute my_example2 = __ATTR(my_example2, 0664, show_file, store_file);

/* Define attribute group */
static struct attribute *my_attributes[] = {
    &my_example1.attr,
    &my_example2.attr,
    NULL // Terminate list
};

static struct attribute_group my_attr_group = {
    .name  = "my_group", // This will create /sys/kernel/my_sysfs/my_group/
    .attrs = my_attributes,
};

/* Open function */
static int my_open(struct inode *inode, struct file *file) {
    pr_info("Device file opened\n");
    return 0;
}

/* Read function */
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    pr_info("Read operation on device\n");
    return 0;
}

/* Write function */
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    pr_info("Write operation on device\n");
    return count;
}

/* Close function */
static int my_close(struct inode *inode, struct file *file) {
    pr_info("Device file closed\n");
    return 0;
}

/* Module initialization */
static int __init my_entry(void) {
    /* Allocate device number */
    if (alloc_chrdev_region(&dev, 0, 1, "my_virtual_device") < 0) {
        pr_err("Failed to allocate Major and Minor number\n");
        return -1;
    }

    pr_info("Major = %d, Minor = %d\n", MAJOR(dev), MINOR(dev));

    /* Initialize character device */
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    /* Add character device */
    if (cdev_add(&my_cdev, dev, 1) < 0) {
        pr_err("Failed to add device to system\n");
        goto r_class;
    }

    /* Create device class */
    if (IS_ERR(my_class = class_create("my_virtual_device"))) {
        pr_info("Cannot create the struct class\n");
        goto r_class;
    }

    /* Create device */
    if (IS_ERR(device_create(my_class, NULL, dev, NULL, "my_virtual_device"))) {
        pr_info("Cannot create the Device\n");
        goto r_device;
    }

    /* Create sysfs kobject */
    my_kobject = kobject_create_and_add("my_sysfs", kernel_kobj);
    if (!my_kobject) {
        pr_err("Failed to create /sys/kernel/my_sysfs\n");
        goto r_sysfs;
    }

    /* Create sysfs attribute group */
    if (sysfs_create_group(my_kobject, &my_attr_group)) {
        pr_err("Failed to create sysfs group\n");
        goto r_sysfs;
    }

    pr_info("Device successfully registered\n");
    return 0;

/* Error handling */
r_sysfs:
    kobject_put(my_kobject);
r_device:
    device_destroy(my_class, dev);
    class_destroy(my_class);
r_class:
    unregister_chrdev_region(dev, 1);
    return -1;
}

/* Module exit function */
void __exit my_exit(void) {
    sysfs_remove_group(my_kobject, &my_attr_group);
    kobject_put(my_kobject);
    device_destroy(my_class, dev);
    cdev_del(&my_cdev);
    class_destroy(my_class);
    unregister_chrdev_region(dev, 1);
    pr_info("Device successfully removed\n");
}

module_init(my_entry);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bojja <balasai@gmail.com>");
MODULE_DESCRIPTION("Simple Linux driver with sysfs interface");
MODULE_VERSION("1.0");

