#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include "message_slot.h"




int main(int argc, char *argv[])
{
    
    unsigned int channel;
    char buffer[MAX_MSG_LEN];
    int file_description, length;

    // Check for correct number of arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <device> <channel>\n", argv[0]);
        return EXIT_FAILURE;
    }
    file_description = open(argv[1], O_RDONLY);
    if (file_description == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    channel = atoi(argv[2]);
    if (ioctl(file_description, IOCTL_SET_CH, channel) == -1) {
        perror("ioctl");
        close(file_description);
        return EXIT_FAILURE;
    }
    length = read(file_description, buffer, MAX_MSG_LEN);
    if (length < 0) {
        perror("read");
        close(file_description);
        return EXIT_FAILURE;
    }
    ssize_t bytes_written = write(STDOUT_FILENO, buffer, length);
    if (bytes_written != length) {
        perror("write");
        close(file_description);
        return EXIT_FAILURE;
    }
    close(file_description);
    return EXIT_SUCCESS;
}
