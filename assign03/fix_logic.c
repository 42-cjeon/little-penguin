// SPDX-License-Identifier: GPL-3.0-or-later

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>

/*
 * sleep `useconds`us `count` times.
 * if useconds is over 20000, using this function strongly discouraged.
 * (see /asm-generic/delay.h)
 */
static u64 long_sleep(u32 count, u32 useconds)
{
    u64 total_sleep = count * useconds;

	for (u32 i = 0; i < count; i++)
		udelay(useconds);

	if (total_sleep > 10000)
		pr_notice("We slept a long time!\n");

	return total_sleep;
}

static int __init my_init(void)
{
	u64 x = long_sleep(10, 100);

    pr_info("slept %lldus\n", x);

	return 0;
}

static void __exit my_exit(void)
{
}

module_init(my_init);
module_exit(my_exit);
