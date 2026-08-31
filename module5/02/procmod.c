#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>  
#include <linux/slab.h>     

#define BUF_SIZE 100        

static char *msg;
static int len;
static int temp;

static ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp)
{
    if (count > temp) {
        count = temp;
    }
    temp -= count;
    copy_to_user(buf, msg, count); 
    if (count == 0) {
        temp = len;
    }
    return count;
}

static ssize_t write_proc(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
    if (count > BUF_SIZE - 1) { 
        count = BUF_SIZE - 1;   
    }
    copy_from_user(msg, buf, count);
    msg[count] = '\0';
    len = count;
    temp = len;
    return count;
}

static const struct proc_ops proc_fops = {
    .proc_read = read_proc,
    .proc_write = write_proc,
};

static int __init procmod_init(void)
{
    msg = kmalloc(BUF_SIZE * sizeof(char), GFP_KERNEL);
    if (!msg) {
        printk(KERN_ALERT "procmod: Failed to allocate memory\n");
        return -ENOMEM;
    }
    msg[0] = '\0';
    len = 0;
    temp = 0;

    proc_create("mydata", 0666, NULL, &proc_fops);
    printk(KERN_INFO "procmod: Module loaded, /proc/mydata created\n");
    return 0;
}

static void __exit procmod_exit(void)
{
    remove_proc_entry("mydata", NULL);
    kfree(msg);
    printk(KERN_INFO "procmod: Module unloaded\n");
}

module_init(procmod_init);
module_exit(procmod_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Якушева Алина");
MODULE_DESCRIPTION("Модуль для обмена данными с userspace через /proc/mydata");
