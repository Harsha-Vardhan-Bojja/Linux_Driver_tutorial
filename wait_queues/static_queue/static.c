#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>                 // kmalloc()
#include <linux/uaccess.h>              // copy_to/from_user()
#include <linux/kthread.h>
#include <linux/wait.h>                 // Required for the wait queues
#include <linux/err.h>

#define NUM_THREADS 3  // Number of threads to create

uint32_t read_count = 0;
static struct task_struct *wait_threads[NUM_THREADS];  // Array of threads

DECLARE_WAIT_QUEUE_HEAD(wait_queue_etx);
dev_t dev = 0;
static struct class *my_class;
static struct cdev my_cdev;
int wait_queue_flag = 0;

/*
** Function Prototypes
*/
static int      __init my_entry(void);
static void     __exit my_exit(void);

/*************** Driver functions **********************/
static int      my_open(struct inode *inode, struct file *file);
static int      my_release(struct inode *inode, struct file *file);
static ssize_t  my_read(struct file *filp, char __user *buf, size_t len, loff_t * off);
static ssize_t  my_write(struct file *filp, const char *buf, size_t len, loff_t * off);

/*
** File operation structure
*/
static struct file_operations fops =
{
        .owner          = THIS_MODULE,
        .read           = my_read,
        .write          = my_write,
        .open           = my_open,
        .release        = my_release,
};

/*
** Thread function
*/
static int wait_function(void *unused)
{
        int thread_id = (int)(intptr_t)unused;  // Get the thread ID from the argument
        pr_info("Thread %d: Started\n", thread_id);

        if (thread_id == 0) {
                pr_info("Thread %d: Waiting using wait_event\n", thread_id);
                wait_event(wait_queue_etx, wait_queue_flag != 0);
                pr_info("Thread %d: Woke up using wait_event\n", thread_id);
        } else if (thread_id == 1) {
                pr_info("Thread %d: Waiting using wait_event_interruptible\n", thread_id);
                wait_event_interruptible(wait_queue_etx, wait_queue_flag != 0);
                pr_info("Thread %d: Woke up using wait_event_interruptible\n", thread_id);
        } else {
                pr_info("Thread %d: Waiting using wait_event_killable\n", thread_id);
                wait_event_killable(wait_queue_etx, wait_queue_flag != 0);
                pr_info("Thread %d: Woke up using wait_event_killable\n", thread_id);
        }
        return 0;
}

/*
** This function will be called when we open the Device file
*/
static int my_open(struct inode *inode, struct file *file)
{
        pr_info("Device File Opened...!!!\n");
        return 0;
}

/*
** This function will be called when we close the Device file
*/
static int my_release(struct inode *inode, struct file *file)
{
        pr_info("Device File Closed...!!!\n");
        return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t my_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
        pr_info("Read Function\n");
        wait_queue_flag = 1;  // Change the condition to wake up the threads
        wake_up(&wait_queue_etx);  // Wake up one thread
        return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t my_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
        pr_info("Write function\n");
        return len;
}

/*
** Module Init function
*/
static int __init my_entry(void)
{
        int i;

        /* Allocating Major number */
        if((alloc_chrdev_region(&dev, 0, 1, "my_virtual_device")) < 0){
                pr_info("Cannot allocate major number\n");
                return -1;
        }
        pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

        /* Creating cdev structure */
        cdev_init(&my_cdev, &fops);
        my_cdev.owner = THIS_MODULE;
        my_cdev.ops = &fops;

        /* Adding character device to the system */
        if((cdev_add(&my_cdev, dev, 1)) < 0){
            pr_info("Cannot add the device to the system\n");
            goto r_class;
        }

        /* Creating struct class */
        if(IS_ERR(my_class = class_create("my_virtual_device"))){
            pr_info("Cannot create the struct class\n");
            goto r_class;
        }

        /* Creating device */
        if(IS_ERR(device_create(my_class, NULL, dev, NULL, "my_virtual_device"))){
            pr_info("Cannot create the Device 1\n");
            goto r_device;
        }

        // Create multiple kernel threads
        for (i = 0; i < NUM_THREADS; i++) {
                wait_threads[i] = kthread_create(wait_function, (void *)(intptr_t)i, "WaitThread%d", i);
                if (wait_threads[i]) {
                        pr_info("Thread %d Created successfully\n", i);
                        wake_up_process(wait_threads[i]);
                } else {
                        pr_info("Thread %d creation failed\n", i);
                }
        }

        pr_info("Device Driver Insert...Done!!!\n");
        return 0;

r_device:
        class_destroy(my_class);
r_class:
        unregister_chrdev_region(dev, 1);
        return -1;
}

/*
** Module exit function
*/
static void __exit my_exit(void)
{
        int i;
        wait_queue_flag = 2;
        wake_up(&wait_queue_etx);

        // Wake up all threads and exit them
        for (i = 0; i < NUM_THREADS; i++) {
                if (wait_threads[i]) {
                        kthread_stop(wait_threads[i]);
                        pr_info("Thread %d Stopped\n", i);
                }
        }

        device_destroy(my_class, dev);
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev, 1);
        pr_info("Device Driver Remove...Done!!!\n");
}

module_init(my_entry);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bojja <balasai@gmail.com>");
MODULE_DESCRIPTION("Simple linux driver with waitqueue and kernel threads");
MODULE_VERSION("1.0");
