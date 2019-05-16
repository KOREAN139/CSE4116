/*
 * Copyright (c) 2019 Sanggu Han
 */
#include <linux/kernel.h>
#include <asm/uaccess.h>

#define RANGE_CHECK(X) (((X) > 0) && ((X) <= 100))
#define TO_NTH_BYTE(X, N)       (((X) & 0xff) << ((sizeof(X) - (N)) * 8))
#define CONSTRUCT_PARAM(P, V, I, L)     ((TO_NTH_BYTE(P, 1)) \
                                        | (TO_NTH_BYTE(V, 2)) \
                                        | (TO_NTH_BYTE(I, 3)) \
                                        | (TO_NTH_BYTE(L, 4)))

struct st_huins_op {
        int interval, lap, op;
};

asmlinkage int sys_huinsw(struct st_huins_op *block)
{
        int i, pos, val;
        struct st_huins_op input;

        copy_from_user(&input, block, sizeof(struct st_huins_op));

        if (!RANGE_CHECK(input.interval) || !RANGE_CHECK(input.lap))
                return -1;

        for (i = input.op, pos = 0; i && !(i % 10); i /= 10)
                pos += 1;
        val = i % 10;

        if (pos > 3 || val > 8 || !val)
                return -1;

        return CONSTRUCT_PARAM(pos, val, input.interval, input.lap);
}
