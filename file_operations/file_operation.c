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

dev_t dev = 0;
struct class *my_class;
struct device *my_device;
struct cdev my_cdev;
int *kernel_buffer;
static int my_open(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file, char __user *buf, size_t count,
                       loff_t *offset);
static ssize_t my_write(struct file *file, const char __user *buf, size_t count,
                        loff_t *offset);
static int my_close(struct inode *inode, struct file *file);
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

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
    pr_err("Cannot allocate mem in the kerenl");
    return -1;
  }
  pr_info("Module successfully opened\n");
  return 0;
}

static ssize_t my_read(struct file *file, char __user *buf, size_t count,
                       loff_t *offset) {

  int ret;
  ret = copy_to_user(buf, kernel_buffer, count);
  if (ret < 0) {
    pr_err("failed to read");
    return ret;
  }
  pr_info("Module successfully read\n");
  return count; 
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count,
                        loff_t *offset) {

  int ret;
  ret = copy_from_user(kernel_buffer, buf, 1024);
  if (ret < 0) {
    pr_err("failed to write");
    return ret;
  }
  pr_info("Module successfully write\n");
  return count;
}

static int my_close(struct inode *inode, struct file *file) {
  kfree(kernel_buffer);
  pr_info("Module successfully closed\n");
  return 0;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  pr_info("IOCTL called\n");
  return 0; 
}

static int __init my_entry(void) {

  int ret;

  ret = alloc_chrdev_region(&dev, 0, 1, "my_virtual_device");
  if (ret < 0) {
    pr_err("Failed to allocate the Major and Minor devices\n");
    return ret;
  }

  my_class = class_create("my_virtual_device");
  if (IS_ERR(my_class)) {
    unregister_chrdev_region(dev, 1);
    pr_err("Failed to allocate the class");
    return PTR_ERR(my_class);
  }

  cdev_init(&my_cdev, &fop);
  ret = cdev_add(&my_cdev, dev, 1);
  if (ret < 0) {
    class_destroy(my_class);
    unregister_chrdev_region(dev, 1);
    pr_err("Cannot add the device");
    return ret;
  }

  my_device = device_create(my_class, NULL, dev, NULL, "my_virtual_device");

  if (IS_ERR(my_device)) {
    cdev_del(&my_cdev);
    class_destroy(my_class);
    unregister_chrdev_region(dev, 1);
    return PTR_ERR(my_device);
  }

  return 0;
}

static void my_exit(void) {

  device_destroy(my_class, dev);
  cdev_del(&my_cdev);
  class_destroy(my_class);
  unregister_chrdev_region(dev, 1);
  pr_err("Device has been removed");
}

module_init(my_entry);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bojja <balasai@gmail.com>");
MODULE_DESCRIPTION("Simple Device Creating File");
MODULE_VERSION("1.0");
