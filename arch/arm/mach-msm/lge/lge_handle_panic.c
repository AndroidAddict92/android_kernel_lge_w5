/*
 * arch/arm/mach-msm/lge/lge_handle_panic.c
 *
 * Copyright (C) 2012 LGE, Inc
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/io.h>
#include <asm/setup.h>
#include <linux/init.h>
#include <mach/subsystem_restart.h>
#include <mach/msm_iomap.h>
#include <mach/lge_handle_panic.h>
#include <mach/restart.h>

#define PANIC_HANDLER_NAME        "panic-handler"

#define RESTART_REASON_ADDR       0x65C
#define DLOAD_MODE_ADDR           0x0
#define UEFI_RAM_DUMP_MAGIC_ADDR  0xC
#define RAM_CONSOLE_ADDR_ADDR     0x24
#define RAM_CONSOLE_SIZE_ADDR     0x28
#define FB1_ADDR_ADDR             0x2C
#define POWERMODE_ADDR            0x30

#define RESTART_REASON      (MSM_IMEM_BASE + RESTART_REASON_ADDR)
#define UEFI_RAM_DUMP_MAGIC \
		(MSM_IMEM_BASE + DLOAD_MODE_ADDR + UEFI_RAM_DUMP_MAGIC_ADDR)
#define RAM_CONSOLE_ADDR    (MSM_IMEM_BASE + RAM_CONSOLE_ADDR_ADDR)
#define RAM_CONSOLE_SIZE    (MSM_IMEM_BASE + RAM_CONSOLE_SIZE_ADDR)
#define FB1_ADDR            (MSM_IMEM_BASE + FB1_ADDR_ADDR)
#define POWERMODE           (MSM_IMEM_BASE + POWERMODE_ADDR)

static int dummy_arg;

static int subsys_crash_magic = 0x0;

int lge_set_magic_subsystem(const char *name, int type)
{
	const char *subsys_name[] = { "adsp", "mba", "modem", "wcnss" };
	int i = 0;

	if (!name)
		return -1;

	for (i = 0; i < ARRAY_SIZE(subsys_name); i++) {
		if (!strncmp(subsys_name[i], name, 40)) {
			subsys_crash_magic = LGE_RB_MAGIC | ((i+1) << 12)
				| type;
			break;
		}
	}

	return 0;
}
EXPORT_SYMBOL(lge_set_magic_subsystem);

void lge_skip_dload_by_sbl(int on)
{
	/* skip entering dload mode at sbl1 */
	__raw_writel(on ? 1 : 0, UEFI_RAM_DUMP_MAGIC);
}
EXPORT_SYMBOL(lge_skip_dload_by_sbl);

void lge_set_ram_console_addr(unsigned int addr, unsigned int size)
{
	__raw_writel(addr, RAM_CONSOLE_ADDR);
	__raw_writel(size, RAM_CONSOLE_SIZE);
}
EXPORT_SYMBOL(lge_set_ram_console_addr);

void lge_set_fb1_addr(unsigned int addr)
{
	__raw_writel(addr, FB1_ADDR);
}
EXPORT_SYMBOL(lge_set_fb1_addr);

void lge_set_panic_reason(void)
{
	if (subsys_crash_magic == 0)
		__raw_writel(LGE_RB_MAGIC | LGE_ERR_KERN, RESTART_REASON);
	else
		__raw_writel(subsys_crash_magic, RESTART_REASON);
}
EXPORT_SYMBOL(lge_set_panic_reason);

void lge_set_restart_reason(unsigned int reason)
{
	__raw_writel(reason, RESTART_REASON);
}
EXPORT_SYMBOL(lge_set_restart_reason);

//                                              
unsigned int set_ram_test_flag=0;
static int set_ram_test(const char *val, struct kernel_param *kp)
{
	set_ram_test_flag=1;
	msm_set_restart_mode(RESTART_DLOAD);
	printk(KERN_CRIT " RAM Test mode!!, Rebooting SoC..\n");
	kernel_restart(NULL);	
	return 0;
}
module_param_call(set_ram_test, set_ram_test, param_get_bool, &dummy_arg,	S_IWUSR | S_IRUGO);
//                                           

static int gen_bug(const char *val, struct kernel_param *kp)
{
	BUG();
	return 0;
}
module_param_call(gen_bug, gen_bug, param_get_bool, &dummy_arg,
		S_IWUSR | S_IRUGO);

static int gen_panic(const char *val, struct kernel_param *kp)
{
	panic("generate test-panic");
	return 0;
}
module_param_call(gen_panic, gen_panic, param_get_bool, &dummy_arg,\
		S_IWUSR | S_IRUGO);

static int gen_adsp_panic(const char *val, struct kernel_param *kp)
{
	subsystem_restart("adsp");
	return 0;
}
module_param_call(gen_adsp_panic, gen_adsp_panic, param_get_bool, &dummy_arg,
		S_IWUSR | S_IRUGO);

static int gen_mba_panic(const char *val, struct kernel_param *kp)
{
	subsystem_restart("mba");
	return 0;
}
module_param_call(gen_mba_panic, gen_mba_panic, param_get_bool, &dummy_arg,
		S_IWUSR | S_IRUGO);

static int gen_modem_panic_type = 0;

int lge_get_modem_panic(void)
{
	return gen_modem_panic_type;
}

EXPORT_SYMBOL(lge_get_modem_panic);

static int gen_modem_panic(const char *val, struct kernel_param *kp)
{
	int ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}
	pr_err("gen_modem_panic param to %d\n", gen_modem_panic_type);
	switch (gen_modem_panic_type) {
		default:
			subsystem_restart("modem");
			break;
		case 2:
			subsys_modem_restart();
			break;			
	}
	return 0;
}
module_param_call(gen_modem_panic, gen_modem_panic, param_get_bool, &gen_modem_panic_type,
		S_IWUSR | S_IRUGO);

static int gen_wcnss_panic(const char *val, struct kernel_param *kp)
{
	subsystem_restart("wcnss");
	return 0;
}
module_param_call(gen_wcnss_panic, gen_wcnss_panic, param_get_bool, &dummy_arg,
		S_IWUSR | S_IRUGO);

#define WDT0_RST        0x04
#define WDT0_EN         0x08
#define WDT0_BARK_TIME  0x10
#define WDT0_BITE_TIME  0x14

extern void __iomem *wdt_timer_get_timer0_base(void);

static int gen_wdt_bark(const char *val, struct kernel_param *kp)
{
	static void __iomem *msm_tmr0_base;
	msm_tmr0_base = wdt_timer_get_timer0_base();

	__raw_writel(0, msm_tmr0_base + WDT0_EN);
	__raw_writel(1, msm_tmr0_base + WDT0_RST);
	__raw_writel(0x31F3, msm_tmr0_base + WDT0_BARK_TIME);
	__raw_writel(5 * 0x31F3, msm_tmr0_base + WDT0_BITE_TIME);
	__raw_writel(1, msm_tmr0_base + WDT0_EN);
	return 0;
}
module_param_call(gen_wdt_bark, gen_wdt_bark, param_get_bool,
		&dummy_arg, S_IWUSR | S_IRUGO);

static int gen_wdt_bite(const char *val, struct kernel_param *kp)
{
	static void __iomem *msm_tmr0_base;
	msm_tmr0_base = wdt_timer_get_timer0_base();
	__raw_writel(0, msm_tmr0_base + WDT0_EN);
	__raw_writel(1, msm_tmr0_base + WDT0_RST);
	__raw_writel(5 * 0x31F3, msm_tmr0_base + WDT0_BARK_TIME);
	__raw_writel(0x31F3, msm_tmr0_base + WDT0_BITE_TIME);
	__raw_writel(1, msm_tmr0_base + WDT0_EN);
	return 0;
}
module_param_call(gen_wdt_bite, gen_wdt_bite, param_get_bool,
		&dummy_arg, S_IWUSR | S_IRUGO);

static int gen_bus_hang(const char *val, struct kernel_param *kp)
{
	static void __iomem *reserved;

	reserved = ioremap(0xFF000000 - 0x10, 0x10);
	__raw_writel(1, reserved);

	return 0;
}
module_param_call(gen_bus_hang, gen_bus_hang, param_get_bool,
		&dummy_arg, S_IWUSR | S_IRUGO);

extern void msm_disable_wdog_debug(void);
static int gen_hw_reset(const char *val, struct kernel_param *kp)
{
	static void __iomem *reserved;

	msm_disable_wdog_debug();
	reserved = ioremap(0xFF000000 - 0x10, 0x10);
	__raw_writel(1, reserved);

	return 0;
}
module_param_call(gen_hw_reset, gen_hw_reset, param_get_bool,
		&dummy_arg, S_IWUSR | S_IRUGO);

#ifdef CONFIG_LGE_CRASH_FOOTPRINT
extern unsigned long int lge_get_crash_footprint(void);
static int get_crash_footprint(char *val, struct kernel_param *kp)
{
    if ((lge_get_crash_footprint() & 0xffff0000) == LGE_RB_MAGIC) {
      if (((lge_get_crash_footprint() & 0x0000f000) == LGE_SUB_ADSP)
        || (lge_get_crash_footprint() & 0x0000f000) == 0) {
          return sprintf(val, "1");
      }
      else
        return sprintf(val, "0");
    }

    return sprintf(val, "0");
}
module_param_call(crash_footprint, NULL, get_crash_footprint,
		&dummy_arg, S_IWUSR | S_IRUGO);
#endif
#include <linux/io.h>
#include <../acpuclock.h>
static int gen_dbi_crash(const char *val, struct kernel_param *kp)
{
    void __iomem *l2_hfpll_address = ioremap(0xF9016000, 4);
    acpuclk_set_rate(0, 576000, SETRATE_CPUFREQ);
    writel(0, l2_hfpll_address);

	return 0;
}
module_param_call(gen_dbi_crash, gen_dbi_crash, param_get_bool,
        &dummy_arg, S_IWUSR | S_IRUGO);

#include <mach/scm.h>
static int gen_dbi_crash_new(const char *val, struct kernel_param *kp)
{
	u8 disable_debug = 0; 
	scm_call(SCM_SVC_BOOT, 8, &disable_debug, sizeof(disable_debug), NULL, 0); 
	return 0;
}
module_param_call(gen_dbi_crash_new, gen_dbi_crash_new, param_get_bool,
        &dummy_arg, S_IWUSR | S_IRUGO);

static int no_powermode=1;
static int no_powermode_set(const char *val, struct kernel_param *kp)
{
    int ret;
    int old_val = no_powermode;

    ret = param_set_int(val, kp);

    if(ret)
        return ret;

    if (no_powermode >> 1) {
        no_powermode = old_val;
        return -EINVAL;
    }

    __raw_writel(no_powermode, POWERMODE);

    return 0;
}
module_param_call(no_powermode, no_powermode_set, param_get_int,
        &no_powermode, S_IWUSR | S_IRUGO);

static int __init lge_panic_handler_probe(struct platform_device *pdev)
{
	int ret = 0;

	return ret;
}

static int __devexit lge_panic_handler_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver panic_handler_driver __refdata = {
	.probe = lge_panic_handler_probe,
	.remove = __devexit_p(lge_panic_handler_remove),
	.driver = {
		.name = PANIC_HANDLER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init lge_panic_handler_init(void)
{
	return platform_driver_register(&panic_handler_driver);
}

static void __exit lge_panic_handler_exit(void)
{
	platform_driver_unregister(&panic_handler_driver);
}

module_init(lge_panic_handler_init);
module_exit(lge_panic_handler_exit);

MODULE_DESCRIPTION("LGE panic handler driver");
MODULE_AUTHOR("SungEun Kim <cleaneye.kim@lge.com>");
MODULE_LICENSE("GPL");
