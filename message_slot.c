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
    msg_node *active_channel;
    bool      censor_enabled;
} file_ctx;

/* release: free file_ctx */
static int slot_release(struct inode *inode, struct file *filp)
{
    kfree(filp->private_data);
    return 0;
}

/* open: allocate file_ctx */
static int slot_open(struct inode *inode, struct file *filp)
{
    file_ctx *ctx = kmalloc(sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;
    ctx->active_channel   = NULL;
    ctx->censor_enabled = false;
    filp->private_data = ctx;
    return 0;
}




/* read: copy the stored message as-is */
static ssize_t slot_read(struct file *file, char __user *buffer,
                         size_t count, loff_t *offset)
{
    file_ctx *ctx = file->private_data;
    msg_node *msg = ctx->active_channel;
    int  i;

    if (!buffer || !msg)
        return -EINVAL;
    if (msg->length == 0)
        return -EWOULDBLOCK;
    if (msg-> length > count)
        return -ENOSPC;

    for (i = 0; i < msg->length; i++)
        if (put_user(msg->data[i], &buffer[i]))
            return -EFAULT;

    return msg->length;
}

/* write: optionally censor, then store */
static ssize_t slot_write(struct file *filp, const char __user *buffer,
                          size_t count, loff_t *off)
{
    file_ctx *ctx = filp->private_data;
    msg_node *msg    = ctx->active_channel;
    int i;
    char temp[MAX_MSG_LEN];

    if (!buffer || !msg)
        return -EINVAL;
    if (count == 0 || count > MAX_MSG_LEN)
        return -EMSGSIZE;
    

    for (i = 0; i < count; i++) {
        if (get_user(temp[i], &buffer[i]))
            return -EFAULT;
        /* apply censorship if enabled: every 3rd byte (0-based idx %3 ==2) */
        if (ctx->censor_enabled && (i % 3) == 2)
            temp[i] = '#';
    }

    /* commit */
    msg->length = count;
    for (i = 0; i < count; i++)
        msg->data[i] = temp[i];

    return count;
}

/* Handle IOCTL operations */
static long slot_ioctl(struct file *filp,
                       unsigned int command, unsigned long val)
{
    file_ctx *ctx = filp->private_data;
    msg_node *current, *previous = NULL, *newn;
    int minor = iminor(filp->f_inode);

    switch (command) {
    case IOCTL_SET_CEN:
        if (val > 1)
            return -EINVAL;
        ctx->censor_enabled = (arg == 1);
        return OKAY;


    case IOCTL_SET_CH:
        if (val == 0)
            return -EINVAL;
        /* find or create channel */
        current = device_slots[minor].head;
        while (current && current->id != arg) {
            previous = current;
            current  = current->next;
        }
        if (!val) {
            newn = kmalloc(sizeof(*newn), GFP_KERNEL);
            if (!newn)
                return -ENOMEM;
            newn->id     = arg;
            newn->length = 0;
            newn->next   = NULL;
            if (!previous)
                device_slots[minor].head = newn;
            else
                prev->next = newn;
            current = newn;
        }
        ctx->active_channel = current;
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
    int idx, res = register_chrdev(MSGSLOT_MAJOR, DRIVER_NAME, &fops);
    if (res < 0)
        return res;
    for (idx = 0; idx < 256; i++)
        device_slots[idx].head = NULL;
    return OKAY;
}

static void __exit slot_exit(void)
{
    int i;
    msg_node *next, *tmp;
    unregister_chrdev(MSGSLOT_MAJOR, DRIVER_NAME);
    for (i = 0; i < 256; i++) {
        next = device_slots[i].head;
        while (next) {
            tmp = next->next;
            kfree(next);
            next = tmp;
        }
    }
}

module_init(slot_init);
module_exit(slot_exit);
