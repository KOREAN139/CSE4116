/*
 * Copyright (c) 2019 Sanggu Han
 */
#ifndef _HUINS_H_
#define _HUINS_H_

#include <linux/ioctl.h>

#define DEVICE_NAME     "dev_driver"
#define MAJOR_NUM       242

#define SUCCESS 0

#define FND_ADDRESS 0x08000004
#define LED_ADDRESS 0x08000016
#define LCD_ADDRESS 0x08000090
#define DOT_ADDRESS 0x08000210

#define IOCTL_RUN_DEVICE _IOR(MAJOR_NUM, 0, int)

#define TO_NTH_BYTE(X, N)       (((X) & 0xff) << ((sizeof(X) - (N)) * 8))
#define GET_NTH_BYTE(X, N)      (((X) >> ((sizeof(X) - (N)) * 8)) & 0xff)

#define CONSTRUCT_PARAM(P, V, I, L)     ((TO_NTH_BYTE(P, 1)) \
                                        | (TO_NTH_BYTE(V, 2)) \
                                        | (TO_NTH_BYTE(I, 3)) \
                                        | (TO_NTH_BYTE(L, 4)))

#define POSITION(X)     (GET_NTH_BYTE(X, 1))
#define VALUE(X)        (GET_NTH_BYTE(X, 2))
#define INTERVAL(X)     (GET_NTH_BYTE(X, 3))
#define LAPS(X)         (GET_NTH_BYTE(X, 4))

#endif
