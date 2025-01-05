#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

dev_t dev = 0;
struct class *my_class;
struct device *my_device;
struct cdev my_cdev;
struct proc_dir_entry *parent;
char *kernel_buffer;

static int my_open_proc(struct inode *inode, struct file *file);
static ssize_t my_read_proc(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t my_write_proc(struct file *file, const char __user *buf, size_t count, loff_t *offset);
static int my_close_proc(struct inode *inode, struct file *file);

static int my_open(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);
static int my_close(struct inode *inode, struct file *file);
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct proc_ops proc_fops = {
    .proc_open = my_open_proc,
    .proc_read = my_read_proc,
    .proc_write = my_write_proc,
    .proc_release = my_close_proc
};

static struct file_operations fop = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_close,
    .unlocked_ioctl = my_ioctl,
};

static int my_open(struct inode *inode, struct file *file) {
    pr_info("Character device opened\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    pr_info("Character device read\n");
    return count;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    pr_info("Character device written\n");
    return count;
}

static int my_close(struct inode *inode, struct file *file) {
    pr_info("Character device closed\n");
    return 0;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    pr_info("IOCTL called\n");
    return 0;
}

static int my_open_proc(struct inode *inode, struct file *file) {
    kernel_buffer = kmalloc(1024, GFP_KERNEL);
    if (!kernel_buffer) {
        pr_err("Failed to allocate memory for /proc entry\n");
        return -ENOMEM;
    }
    pr_info("/proc entry opened\n");
    return 0;
}

static ssize_t my_read_proc(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    size_t buffer_size = strlen(kernel_buffer);

    // If offset is beyond the data size, return EOF
    if (*offset >= buffer_size)
        return 0;

    // Limit count to the remaining data
    if (count > buffer_size - *offset)
        count = buffer_size - *offset;

    // Copy data from kernel_buffer to user buffer
    if (copy_to_user(buf, kernel_buffer + *offset, count)) {
        pr_err("Failed to copy data to user buffer\n");
        return -EFAULT;
    }

    // Update offset
    *offset += count;

    pr_info("Read %zu bytes from kernel_buffer\n", count);
    return count;
}

static ssize_t my_write_proc(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    if (count > 1024) {
        pr_err("Write exceeds buffer size\n");
        return -EINVAL;
    }

    if (copy_from_user(kernel_buffer, buf, count)) {
        pr_err("Failed to write to /proc entry\n");
        return -EFAULT;
    }

    pr_info("/proc entry written\n");
    return count;
}

static int my_close_proc(struct inode *inode, struct file *file) {
    kfree(kernel_buffer);
    pr_info("/proc entry closed\n");
    return 0;
}

static int __init my_entry(void) {
    int ret;

    ret = alloc_chrdev_region(&dev, 0, 1, "bojja_virtual_device");
    if (ret < 0) {
        pr_err("Failed to allocate Major and Minor numbers\n");
        return ret;
    }

    my_class = class_create("bojja_virtual_device");
    if (IS_ERR(my_class)) {
        unregister_chrdev_region(dev, 1);
        pr_err("Failed to create device class\n");
        return PTR_ERR(my_class);
    }

    cdev_init(&my_cdev, &fop);
    ret = cdev_add(&my_cdev, dev, 1);
    if (ret < 0) {
        class_destroy(my_class);
        unregister_chrdev_region(dev, 1);
        pr_err("Failed to add cdev\n");
        return ret;
    }

    my_device = device_create(my_class, NULL, dev, NULL, "bojja_virtual_device");
    if (IS_ERR(my_device)) {
        cdev_del(&my_cdev);
        class_destroy(my_class);
        unregister_chrdev_region(dev, 1);
        pr_err("Failed to create device\n");
        return PTR_ERR(my_device);
    }

    parent = proc_mkdir("bojja_proc_dir", NULL);
    if (!parent) {
        pr_err("Failed to create /proc directory\n");
        device_destroy(my_class, dev);
        cdev_del(&my_cdev);
        class_destroy(my_class);
        unregister_chrdev_region(dev, 1);
        return -ENOMEM;
    }

    if (!proc_create("bojja_proc", 0666, parent, &proc_fops)) {
        pr_err("Failed to create /proc/bojja_proc entry\n");
        proc_remove(parent);
        device_destroy(my_class, dev);
        cdev_del(&my_cdev);
        class_destroy(my_class);
        unregister_chrdev_region(dev, 1);
        return -ENOMEM;
    }

    pr_info("Module initialized successfully\n");
    return 0;
}

static void __exit my_exit(void) {
    proc_remove(parent);
    device_destroy(my_class, dev);
    cdev_del(&my_cdev);
    class_destroy(my_class);
    unregister_chrdev_region(dev, 1);
    pr_info("Module exited successfully\n");
}

module_init(my_entry);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bojja <balasai@gmail.com>");
MODULE_DESCRIPTION("Improved Driver Example");
MODULE_VERSION("1.1");

