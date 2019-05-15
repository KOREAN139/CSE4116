#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/ioctl.h>
#include "huins.h"

struct st_huins_op {
        int interval, lap, op;
};

int main(int argc, char **argv) {
        int fd, param, ret;
        struct st_huins_op input;

        if (argc != 4) {
                puts("Usage: ./app <interval> <lap> <option>");
                return -1;
        }

        input.interval = atoi(argv[1]);
        input.lap = atoi(argv[2]);
        input.op = atoi(argv[3]);

        param = syscall(__NR_huinsw, &input);
        if (param < 0) {
                puts("Invalid input has given");
                puts("Interval [1, 100] Lap [1, 100] Option [0001, 8000]\n");
                return -1;
        }
        
        fd = open(DEVICE_NAME, 0);
        if (fd < 0) {
                printf("Can't open device file: %s\n", DEVICE_NAME);
                return -1;
        }
        
        ret = ioctl(fd, IOCTL_RUN_DEVICE, param);
        if (ret < 0) {
                printf("IOCTL_RUN_DEVICE failed:%d\n", ret);
                return -1;
        }

        close(fd);

        return 0;
}
