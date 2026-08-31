#include <linux/module.h>
#include <linux/kernel.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/net_namespace.h>

#define NETLINK_USER 31

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Якушева Алина");
MODULE_DESCRIPTION("Модуль для обмена данными с userspace через Netlink");

static struct sock *nl_sk = NULL;

static void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    char *msg = "Hello from kernel!";
    int res;

    printk(KERN_INFO "Netlink: Получено сообщение\n");

    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid;

    printk(KERN_INFO "Netlink: Сообщение от пользователя: %s\n", (char *)nlmsg_data(nlh));

    msg_size = strlen(msg);
    skb_out = nlmsg_new(msg_size, GFP_KERNEL);
    if (!skb_out) {
        printk(KERN_ERR "Netlink: Ошибка выделения памяти для ответа\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0;
    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res < 0) {
        printk(KERN_INFO "Netlink: Ошибка отправки ответа: %d\n", res);
    } else {
        printk(KERN_INFO "Netlink: Ответ отправлен\n");
    }
}

static struct netlink_kernel_cfg cfg = {
    .input = nl_recv_msg,
};

static int __init netlinkmod_init(void)
{
    printk(KERN_INFO "Netlink: Загрузка модуля...\n");

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        printk(KERN_ERR "Netlink: Ошибка создания сокета\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "Netlink: Модуль загружен, сокет создан\n");
    return 0;
}

static void __exit netlinkmod_exit(void)
{
    printk(KERN_INFO "Netlink: Выгрузка модуля...\n");
    netlink_kernel_release(nl_sk);
    printk(KERN_INFO "Netlink: Модуль выгружен\n");
}

module_init(netlinkmod_init);
module_exit(netlinkmod_exit);
