#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/inet.h>
#include <linux/skbuff.h>

#define PROC_NAME "firewall_ips"
#define MAX_IPS 32
#define IP_STR_LEN 16

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Якушева Алина");
MODULE_DESCRIPTION("Модуль для фильтрации исходящих пакетов по IP (Netfilter)");

static char blocked_ips[MAX_IPS][IP_STR_LEN];
static int ip_count = 0;

static unsigned int firewall_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
    struct iphdr *ip_header;
    char ip_str[IP_STR_LEN];
    int i;

    if (!skb || !skb_network_header(skb))
        return NF_ACCEPT;

    ip_header = (struct iphdr *)skb_network_header(skb);

    if (ip_header->version != 4)
        return NF_ACCEPT;

    snprintf(ip_str, IP_STR_LEN, "%pI4", &ip_header->daddr);

    for (i = 0; i < ip_count; i++) {
        if (strcmp(ip_str, blocked_ips[i]) == 0) {
            printk(KERN_INFO "Firewall: Блокирован пакет на %s\n", ip_str);
            return NF_DROP;
        }
    }

    return NF_ACCEPT;
}

static ssize_t firewall_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    char page[512];
    int pos = 0;
    int i;

    if (*offset > 0)
        return 0;
    pos += snprintf(page + pos, sizeof(page) - pos, "Всего адресов: %d\n", ip_count);
    for (i = 0; i < ip_count; i++) { pos += snprintf(page + pos, sizeof(page) - pos, "%s\n", blocked_ips[i]);
    }
    if (copy_to_user(buf, page, pos))
        return -EFAULT;

    *offset = pos;
    return pos;
}

static ssize_t firewall_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    char command[64];
    char ip[IP_STR_LEN];
    int i;

    if (len >= sizeof(command))
        return -EINVAL;
    if (copy_from_user(command, buf, len))
        return -EFAULT;
    command[len] = '\0';
    if (command[len - 1] == '\n')
        command[len - 1] = '\0';
    if (strncmp(command, "add ", 4) == 0) {
        strcpy(ip, command + 4);

        if (ip_count >= MAX_IPS)
            return -ENOSPC;

        for (i = 0; i < ip_count; i++) {
            if (strcmp(blocked_ips[i], ip) == 0)
                return len;
        }

        strcpy(blocked_ips[ip_count], ip);
        ip_count++;
        printk(KERN_INFO "Firewall: Добавлен IP %s\n", ip);
        return len;
    }

    if (strncmp(command, "del ", 4) == 0) {
        strcpy(ip, command + 4);

        for (i = 0; i < ip_count; i++) {
            if (strcmp(blocked_ips[i], ip) == 0) {
                for (; i < ip_count - 1; i++)
                    strcpy(blocked_ips[i], blocked_ips[i + 1]);
                ip_count--;
                printk(KERN_INFO "Firewall: Удалён IP %s\n", ip);
                return len;
            }
        }
        printk(KERN_WARNING "Firewall: IP %s не найден\n", ip);
        return len;
    }

    if (strcmp(command, "clear") == 0) {
        ip_count = 0;
        printk(KERN_INFO "Firewall: Список очищен\n");
        return len;
    }

    return -EINVAL;
}

static const struct proc_ops proc_fops = {
    .proc_read = firewall_read,
    .proc_write = firewall_write,
};

static struct nf_hook_ops nf_hook_ops;

static int __init firewall_init(void)
{
    nf_hook_ops.hook = firewall_hook;
    nf_hook_ops.hooknum = NF_INET_POST_ROUTING;
    nf_hook_ops.pf = PF_INET;
    nf_hook_ops.priority = NF_IP_PRI_FIRST;

    if (nf_register_net_hook(&init_net, &nf_hook_ops)) {
        printk(KERN_ERR "Firewall: Ошибка регистрации хука\n");
        return -ENOMEM;
    }

    if (!proc_create(PROC_NAME, 0666, NULL, &proc_fops)) {
        printk(KERN_ERR "Firewall: Ошибка создания /proc/%s\n", PROC_NAME);
        nf_unregister_net_hook(&init_net, &nf_hook_ops);
        return -ENOMEM;
    }

    printk(KERN_INFO "Firewall: Модуль загружен\n");
printk(KERN_INFO "Firewall: Управление через /proc/%s\n", PROC_NAME);
    return 0;
}

static void __exit firewall_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    nf_unregister_net_hook(&init_net, &nf_hook_ops);
    ip_count = 0;

    printk(KERN_INFO "Firewall: Модуль выгружен\n");
}

module_init(firewall_init);
module_exit(firewall_exit);
