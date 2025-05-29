/* msgslot.h */
#ifndef MSGSLOT_H
#define MSGSLOT_H

#include <linux/ioctl.h>

#define MSGSLOT_MAJOR     235
#define IOCTL_SET_CH      _IOW(MSGSLOT_MAJOR, 0, unsigned int)
#define IOCTL_SET_CEN     _IOW(MSGSLOT_MAJOR, 1, unsigned int)

#define DRIVER_NAME       "msgslot"
#define MAX_MSG_LEN       128
#define OKAY              0

typedef struct msg_node {
    unsigned int      id;
    char              data[MAX_MSG_LEN];
    int               length;
    struct msg_node  *next;
} msg_node;

typedef struct msg_list {
    msg_node *head;
} msg_list;

#endif /* MSGSLOT_H */
