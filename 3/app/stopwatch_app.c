/*
 * Copyright (c) 2019 Sanggu Han
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "stopwatch_app.h"

int main(int argc, char **argv)
{
        int fd = open(DEVICE_NAME, 0);
        if (fd < 0) {
                printf("Can't open device file: %s\n", DEVICE_NAME);
                return -1;
        }

        write(fd, NULL, 0);

        close(fd);

        return 0;
}
