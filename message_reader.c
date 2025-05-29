/* reader.c */
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include "message_slot.h"

int main(int argc, char *argv[])
{
    int fd, len;
    unsigned int ch;
    char buf[MAX_MSG_LEN];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <device> <channel>\n", argv[0]);
        return 1;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    ch = atoi(argv[2]);
    if (ioctl(fd, IOCTL_SET_CH, ch) < 0) {
        perror("ioctl");
        close(fd);
        return 1;
    }
    len = read(fd, buf, MAX_MSG_LEN);
    if (len < 0) {
        perror("read");
        close(fd);
        return 1;
    }
    if (write(STDOUT_FILENO, buf, len) != len) {
        perror("write");
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}
