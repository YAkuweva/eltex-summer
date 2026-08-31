#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define DEVICE_NAME "chardev"
#define BUF_SIZE 128

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Якушева Алина");
MODULE_DESCRIPTION("Символьный драйвер для обмена данными с userspace");

static int major_number;
static struct class *chardev_class;
static struct device *chardev_device;
static char *device_buffer;
static int buffer_len;

static int chardev_open(struct inode *inode, struct file *file);
static int chardev_release(struct inode *inode, struct file *file);
static ssize_t chardev_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t chardev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

static const struct file_operations chardev_fops = {
    .owner = THIS_MODULE,
    .open = chardev_open,
    .release = chardev_release,
    .read = chardev_read,
    .write = chardev_write,
};

static int __init chardev_init(void)
{
    int error;

    device_buffer = kmalloc(BUF_SIZE * sizeof(char), GFP_KERNEL);
    if (!device_buffer) {
        printk(KERN_ERR "chardev: Ошибка выделения памяти\n");
        return -ENOMEM;
    }
    device_buffer[0] = '\0';
    buffer_len = 0;

    error = register_chrdev(0, DEVICE_NAME, &chardev_fops);
    if (error < 0) {
        printk(KERN_ERR "chardev: Ошибка регистрации устройства: %d\n", error);
        kfree(device_buffer);
        return error;
    }
    major_number = error;

    chardev_class = class_create(DEVICE_NAME);
    if (IS_ERR(chardev_class)) {
        printk(KERN_ERR "chardev: Ошибка создания класса\n");
        unregister_chrdev(major_number, DEVICE_NAME);
        kfree(device_buffer);
        return PTR_ERR(chardev_class);
    }

    chardev_device = device_create(chardev_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(chardev_device)) {
        printk(KERN_ERR "chardev: Ошибка создания устройства\n");
        class_destroy(chardev_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        kfree(device_buffer);
        return PTR_ERR(chardev_device);
    }

    printk(KERN_INFO "chardev: Модуль загружен, /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit chardev_exit(void)
{
    device_destroy(chardev_class, MKDEV(major_number, 0));
    class_destroy(chardev_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    kfree(device_buffer);

    printk(KERN_INFO "chardev: Модуль выгружен\n");
}

static int chardev_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int chardev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t chardev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int bytes_to_read;

    if (*offset >= buffer_len) {
        return 0;
    }

    bytes_to_read = buffer_len - *offset;
    if (count < bytes_to_read) {
        bytes_to_read = count;
    }

    if (copy_to_user(buf, device_buffer + *offset, bytes_to_read)) {
        return -EFAULT;
    }

    *offset += bytes_to_read;
    return bytes_to_read;
}

static ssize_t chardev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    int bytes_to_write;

    if (count >= BUF_SIZE) {
        bytes_to_write = BUF_SIZE - 1;
    } else {
        bytes_to_write = count;
    }

    if (copy_from_user(device_buffer, buf, bytes_to_write)) {
        return -EFAULT;
    }

    device_buffer[bytes_to_write] = '\0';
    buffer_len = bytes_to_write;

    return bytes_to_write;
}

module_init(chardev_init);
module_exit(chardev_exit);
