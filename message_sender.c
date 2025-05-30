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
    int file_d, w;
    unsigned int ch, censor;

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <device> <channel> <censor (0|1)> <message>\n", argv[0]);
        return EXIT_FAILURE;
    }
    file_d = open(argv[1], O_WRONLY);
    if (file_d < 0) { perror("open"); return 1; }

    ch = atoi(argv[2]);
    censor = atoi(argv[3]);

    if (ioctl(file_d, IOCTL_SET_CEN, censor) < 0) {
        perror("ioctl(censor)");
        close(file_d);
        return EXIT_FAILURE;
    }
    if (ioctl(file_d, IOCTL_SET_CH, ch) < 0) {
        perror("ioctl(channel)");
        close(file_d);
        return EXIT_FAILURE;
    }

    w = write(file_d, argv[4], strlen(argv[4]));
    if (w < 0) {
        perror("write");
        close(file_d);
        return EXIT_FAILURE;
    }
    close(file_d);
    return EXIT_SUCCESS;
}
