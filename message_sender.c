/* sender.c */
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "message_slot.h"

int main(int argc, char *argv[])
{
    int fd, written;
    unsigned int ch, cen;

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <device> <channel> <censor (0|1)> <message>\n", argv[0]);
        return 1;
    }
    fd = open(argv[1], O_WRONLY);
    if (fd < 0) { perror("open"); return 1; }

    ch = atoi(argv[2]);
    cen = atoi(argv[3]);

    if (ioctl(fd, IOCTL_SET_CEN, cen) < 0) {
        perror("ioctl(censor)");
        close(fd);
        return 1;
    }
    if (ioctl(fd, IOCTL_SET_CH, ch) < 0) {
        perror("ioctl(channel)");
        close(fd);
        return 1;
    }

    written = write(fd, argv[4], strlen(argv[4]));
    if (written < 0) {
        perror("write");
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}
