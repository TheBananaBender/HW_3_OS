/* msgslot.c */
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "message_slot.h"

MODULE_LICENSE("GPL");

/* Per‐minor list of channels */
static msg_list device_slots[256];

/* Per‐fd context: selected channel + censorship flag */
typedef struct file_ctx {
    msg_node *chan;
    bool      censor;
} file_ctx;

/* open: allocate file_ctx */
static int slot_open(struct inode *inode, struct file *filp)
{
    file_ctx *ctx = kmalloc(sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;
    ctx->chan   = NULL;
    ctx->censor = false;
    filp->private_data = ctx;
    return 0;
}

/* release: free file_ctx */
static int slot_release(struct inode *inode, struct file *filp)
{
    kfree(filp->private_data);
    return 0;
}

/* read: copy the stored message as-is */
static ssize_t slot_read(struct file *filp, char __user *buf,
                         size_t count, loff_t *off)
{
    file_ctx *ctx = filp->private_data;
    msg_node *m    = ctx->chan;
    int  i;

    if (!buf || !m)
        return -EINVAL;
    if (m->length == 0)
        return -EWOULDBLOCK;
    if (m->length > count)
        return -ENOSPC;

    for (i = 0; i < m->length; i++)
        if (put_user(m->data[i], &buf[i]))
            return -EFAULT;

    return m->length;
}

/* write: optionally censor, then store */
static ssize_t slot_write(struct file *filp, const char __user *buf,
                          size_t count, loff_t *off)
{
    file_ctx *ctx = filp->private_data;
    msg_node *m    = ctx->chan;
    char temp[MAX_MSG_LEN];
    int i;

    if (!buf || !m)
        return -EINVAL;
    if (count == 0 || count > MAX_MSG_LEN)
        return -EMSGSIZE;

    /* copy into temp */
    for (i = 0; i < count; i++) {
        if (get_user(temp[i], &buf[i]))
            return -EFAULT;
        /* apply censorship if enabled: every 3rd byte (0-based idx %3 ==2) */
        if (ctx->censor && (i % 3) == 2)
            temp[i] = '#';
    }

    /* commit */
    m->length = count;
    for (i = 0; i < count; i++)
        m->data[i] = temp[i];

    return count;
}

/* ioctl: handle SET_CEN and SET_CH */
static long slot_ioctl(struct file *filp,
                       unsigned int cmd, unsigned long arg)
{
    file_ctx *ctx = filp->private_data;
    msg_node *cur, *prev = NULL, *newn;
    int minor = iminor(filp->f_inode);

    switch (cmd) {
    case IOCTL_SET_CEN:
        if (arg > 1)
            return -EINVAL;
        ctx->censor = (arg == 1);
        return OKAY;


    case IOCTL_SET_CH:
        if (arg == 0)
            return -EINVAL;
        /* find or create channel */
        cur = device_slots[minor].head;
        while (cur && cur->id != arg) {
            prev = cur;
            cur  = cur->next;
        }
        if (!cur) {
            newn = kmalloc(sizeof(*newn), GFP_KERNEL);
            if (!newn)
                return -ENOMEM;
            newn->id     = arg;
            newn->length = 0;
            newn->next   = NULL;
            if (!prev)
                device_slots[minor].head = newn;
            else
                prev->next = newn;
            cur = newn;
        }
        ctx->chan = cur;
        return OKAY;

    default:
        return -EINVAL;
    }
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = slot_open,
    .release        = slot_release,
    .read           = slot_read,
    .write          = slot_write,
    .unlocked_ioctl = slot_ioctl,
};

static int __init slot_init(void)
{
    int i, rc = register_chrdev(MSGSLOT_MAJOR, DRIVER_NAME, &fops);
    if (rc < 0)
        return rc;
    for (i = 0; i < 256; i++)
        device_slots[i].head = NULL;
    return OKAY;
}

static void __exit slot_exit(void)
{
    int i;
    msg_node *n, *tmp;
    unregister_chrdev(MSGSLOT_MAJOR, DRIVER_NAME);
    for (i = 0; i < 256; i++) {
        n = device_slots[i].head;
        while (n) {
            tmp = n->next;
            kfree(n);
            n = tmp;
        }
    }
}

module_init(slot_init);
module_exit(slot_exit);
