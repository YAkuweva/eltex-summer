#include <linux/module.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/console_struct.h>
#include <linux/vt_kern.h>
#include <linux/timer.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>

#define BLINK_DELAY HZ/5
#define RESTORE_LEDS 0xFF

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Якушева Алина");
MODULE_DESCRIPTION("Модуль для мигания лампочек клавиатуры через sysfs");

static struct timer_list my_timer;
static struct tty_driver *my_driver;
static int blink_mode = 0;
static int current_leds = 0;

static void my_timer_func(struct timer_list *ptr)
{
    if (current_leds == blink_mode) {
        current_leds = RESTORE_LEDS;
    } else {
        current_leds = blink_mode;
    }
    
    (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, current_leds);
    
    my_timer.expires = jiffies + BLINK_DELAY;
    add_timer(&my_timer);
}

static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", blink_mode);
}

static ssize_t mode_store(struct kobject *kobj, struct kobj_attribute *attr, 
                          const char *buf, size_t count)
{
    int new_mode;
    if (sscanf(buf, "%d", &new_mode) != 1) {
        return -EINVAL;
    }
    
    if (new_mode >= 0 && new_mode <= 7) {
        blink_mode = new_mode;
        if (blink_mode == 0) {
            current_leds = RESTORE_LEDS;
            (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, current_leds);
        }
    }
    return count;
}

static struct kobj_attribute mode_attribute = __ATTR(mode, 0664, mode_show, mode_store);

static int __init kbleds_init(void)
{
    struct kobject *kbleds_kobject;
    int error = 0;
    
    printk(KERN_INFO "kbleds: Загрузка модуля...\n");
    
    if (!vc_cons[fg_console].d) {
        printk(KERN_ERR "kbleds: Нет активной консоли\n");
        return -ENODEV;
    }
    
    my_driver = vc_cons[fg_console].d->port.tty->driver;
    if (!my_driver) {
        printk(KERN_ERR "kbleds: Не удалось получить драйвер консоли\n");
        return -ENODEV;
    }
    
    /* Создаем директорию в sysfs: /sys/kernel/kbleds */
    kbleds_kobject = kobject_create_and_add("kbleds", kernel_kobj);
    if (!kbleds_kobject) {
        printk(KERN_ERR "kbleds: Не удалось создать kobject\n");
        return -ENOMEM;
    }
    
    error = sysfs_create_file(kbleds_kobject, &mode_attribute.attr);
    if (error) {
        printk(KERN_ERR "kbleds: Не удалось создать файл в sysfs\n");
        kobject_put(kbleds_kobject);
        return error;
    }
    
    timer_setup(&my_timer, my_timer_func, 0);
    my_timer.expires = jiffies + BLINK_DELAY;
    add_timer(&my_timer);
    
    printk(KERN_INFO "kbleds: Модуль загружен. Управление через /sys/kernel/kbleds/mode\n");
    return 0;
}

static void __exit kbleds_cleanup(void)
{
    printk(KERN_INFO "kbleds: Выгрузка модуля...\n");
    
    timer_shutdown_sync(&my_timer);
    
    (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, RESTORE_LEDS);
    
    sysfs_remove_file(kernel_kobj->parent, &mode_attribute.attr);
    
    printk(KERN_INFO "kbleds: Модуль выгружен\n");
}

module_init(kbleds_init);
module_exit(kbleds_cleanup);
