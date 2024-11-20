#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static int value;
static int arr[5];
static char *name;
static int value_new=0;

module_param(value, int, S_IRUGO | S_IWUSR);
module_param_array(arr, int, NULL, S_IRUGO | S_IWUSR);
module_param(name, charp, S_IRUGO | S_IWUSR);

static int notify_value(const char *val, const struct kernel_param *kp) {
    int result = param_set_int(val, kp);
    if (result == 0) {
        printk(KERN_INFO "Variable value has been updated\n");
        printk(KERN_INFO "The updated value_new is = %d\n", value_new);
        return 0;
    } else {
        return -1;
    }
}
static int my_get_value(char *buffer, const struct kernel_param *kp){
    int result = snprintf(buffer, PAGE_SIZE, "%d\n", value_new);
    
    // Log the buffer content to the kernel log
    if (result >= 0) {
        printk(KERN_INFO "Buffer content = %s", buffer);
    } else {
        printk(KERN_ERR "Failed to write to buffer");
    }

    return result;  // Return the number of characters written
}

struct kernel_param_ops my_param_ops = {
    .set = notify_value,
    .get = my_get_value,
};



// Ensure this line is not conflicting with the previous `module_param` for `value`
module_param_cb(value_new, &my_param_ops, &value_new, S_IRUGO | S_IWUSR);

static int __init entry(void) {
    printk(KERN_INFO "Module successfully mounted\n");
    printk(KERN_INFO "Module written by: %s\n", name);
    printk(KERN_INFO "The value = %d\n", value);
    printk(KERN_INFO "The array values are:\n");
    printk(KERN_INFO "The vlaue_new = %d\n", value_new);
    for (int i = 0; i < sizeof(arr) / sizeof(int); i++) {
        printk(KERN_INFO "arr[%d] = %d\n", i, arr[i]);
    }
    return 0;
}

static void __exit cleanup(void) {
    printk(KERN_INFO "Module successfully unmounted\n");
}

module_init(entry);
module_exit(cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bojja <balasai@gmail.com>");
MODULE_DESCRIPTION("Simple hello world driver");
MODULE_VERSION("6:1.0");

