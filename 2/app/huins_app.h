/*
 * Copyright (c) 2019 Sanggu Han
 */
#ifndef _HUINS_APP_H_
#define _HUINS_APP_H_

#include <sys/ioctl.h>

#define DEVICE_NAME     "/dev/dev_driver"
#define MAJOR_NUM       242

#define SUCCESS 0

#define IOCTL_RUN_DEVICE _IOR(MAJOR_NUM, 0, int)

#endif
