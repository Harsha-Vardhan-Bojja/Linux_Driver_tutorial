#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/err.h>

dev_t dev = 0;
struct class *my_class;
struct device *my_device;

static int __init my_dev_create(void) {
    int ret;

    ret = alloc_chrdev_region(&dev, 0, 1, "dummy");
    if (ret < 0) {
        pr_err("Cannot allocate major and minor numbers for the devices\n");
        return ret;  
    }

    my_class = class_create( "dummy");
    if (IS_ERR(my_class)) {
        pr_err("Failed to create the class\n");
        unregister_chrdev_region(dev, 1); 
        return PTR_ERR(my_class); 
    }

    
        my_device = device_create(my_class, NULL, dev, NULL, "dummy");
        if (IS_ERR(my_device)) {
            pr_err("Failed to create device\n");
            device_destroy(my_class, dev);
            class_destroy(my_class);
            unregister_chrdev_region(dev, 1);  
            return PTR_ERR(my_device);  
        }
        pr_info("dummy has successfully created\n");  
   

    return 0; 
}

static void __exit my_dev_delete(void) {
    
    device_destroy(my_class, dev);
    class_destroy(my_class);
    unregister_chrdev_region(dev, 1);  
    pr_info("Devices have successfully removed\n");
}

module_init(my_dev_create);
module_exit(my_dev_delete);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bojja <balasai3903@gmail.com>");
MODULE_DESCRIPTION("Simple Device creating file");
MODULE_VERSION("6:1.0");

