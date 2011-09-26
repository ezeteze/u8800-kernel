/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/bootmem.h>
#include <linux/io.h>
#ifdef CONFIG_SPI_QSD
#include <linux/spi/spi.h>
#endif
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/marimba.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/ofn_atlab.h>
#include <linux/power_supply.h>
#include <linux/input/pmic8058-keypad.h>
#include <linux/i2c/isa1200.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <linux/i2c/tsc2007.h>
#include <linux/input/kp_flip_switch.h>
#include <linux/leds-pmic8058.h>
#include <linux/input/cy8c_ts.h>
#include <linux/msm_adc.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>

#include <mach/mpp.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/camera.h>
#include <mach/memory.h>
#include <mach/msm_iomap.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/msm_spi.h>
#include <mach/qdsp5v2/msm_lpa.h>
#include <mach/dma.h>
#include <linux/android_pmem.h>
#include <linux/input/msm_ts.h>
#include <mach/pmic.h>
#include <mach/rpc_pmapp.h>
#include <mach/qdsp5v2/aux_pcm.h>
#include <mach/msm_battery.h>
#include <mach/rpc_server_handset.h>
#include <mach/msm_tsif.h>
#include <linux/cyttsp.h>
#ifdef CONFIG_USB_AUTO_INSTALL
#include "../../../drivers/usb/gadget/usb_switch_huawei.h"
#include "../../../arch/arm/mach-msm/proc_comm.h"
#include "smd_private.h"

#define USB_SERIAL_LEN 20
/* keep the parameters transmitted from SMEM */
smem_huawei_vender usb_para_data;

/* keep the boot mode transfered from APPSBL */
//unsigned int usb_boot_mode = 0;

/* keep usb parameters transfered from modem */
app_usb_para usb_para_info;

/* all the pid used by mobile */
/* add new pid config for google */
/* new requirement: usb tethering */
usb_pid_stru usb_pid_array[]={
    {PID_ONLY_CDROM,     PID_NORMAL,     PID_UDISK, PID_AUTH,     PID_GOOGLE, PID_WLAN}, /* for COMMON products */
    {PID_ONLY_CDROM_TMO, PID_NORMAL_TMO, PID_UDISK, PID_AUTH_TMO, PID_GOOGLE, PID_WLAN}, /* for TMO products */
};

/* pointer to the member of usb_pid_array[], according to the current product */
usb_pid_stru *curr_usb_pid_ptr = &usb_pid_array[0];
#endif  

#ifdef CONFIG_HUAWEI_FEATURE_AT42QT_TS
#include <linux/atmel_i2c_rmi.h>
#endif
#include <linux/audio_amplifier.h>
#include <asm/mach/mmc.h>
#include <asm/mach/flash.h>
#include <mach/vreg.h>
#include "devices.h"
#include "timer.h"
#include "socinfo.h"
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android.h>
#endif
#include "pm.h"
#include "spm.h"
#include <linux/msm_kgsl.h>
#include <mach/dal_axi.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_reqs.h>

#ifdef CONFIG_HUAWEI_FEATURE_VIBRATOR
#include "msm_vibrator.h"
#endif

#include <linux/hardware_self_adapt.h>

#ifdef CONFIG_HUAWEI_EVALUATE_POWER_CONSUMPTION 
#include <mach/msm_battery.h>
#endif

#define MSM_PMEM_SF_SIZE	0x1700000
#define MSM_FB_SIZE		0x500000
#define MSM_GPU_PHYS_SIZE       0x200000
/*add dsp memory space for video*/
#define MSM_PMEM_ADSP_SIZE      0x3000000
#define MSM_FLUID_PMEM_ADSP_SIZE	0x2800000
#define PMEM_KERNEL_EBI1_SIZE   0x600000
#define MSM_PMEM_AUDIO_SIZE     0x200000

#define PMIC_GPIO_INT		27
#define PMIC_VREG_WLAN_LEVEL	2900
#define PMIC_GPIO_SD_DET	36
#define PMIC_GPIO_SDC4_EN	17  /* PMIC GPIO Number 18 */

#define FPGA_SDCC_STATUS       0x8E0001A8

#define FPGA_OPTNAV_GPIO_ADDR	0x8E000026

#define OPTNAV_CHIP_SELECT	19
/*this is i2c pull-up power configs the i2c 
*pinname is gp13 and the voltage of the pin is 1800 mv */
#ifdef CONFIG_HUAWEI_KERNEL
	#define VREG_GP13_NAME	"gp13" 
	#define VREG_GP13_VOLTAGE_VALUE	1800
#endif
#ifdef CONFIG_HUAWEI_FEATURE_AT42QT_TS
#define VCC_TS2V8 "gp4"
#define VCC_TS1V8 "gp7"
#endif

/* Macros assume PMIC GPIOs start at 0 */
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)     (pm_gpio + NR_GPIO_IRQS)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)    (sys_gpio - NR_GPIO_IRQS)

#define PMIC_GPIO_HAP_ENABLE   16  /* PMIC GPIO Number 17 */

#define HAP_LVL_SHFT_MSM_GPIO 24

#define	PM_FLIP_MPP 5 /* PMIC MPP 06 */

static unsigned int camera_id = 0;
static unsigned int lcd_id = 0;
static unsigned int ts_id = 0;
static unsigned int sub_board_id = 0;
#ifdef CONFIG_HUAWEI_POWER_DOWN_CHARGE
static unsigned int charge_flag = 0;
#endif

static int pm8058_gpios_init(void)
{
	int rc;
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	struct pm8058_gpio sdcc_det = {
		.direction      = PM_GPIO_DIR_IN,
		.pull           = PM_GPIO_PULL_UP_1P5,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};
#endif
	struct pm8058_gpio sdc4_en = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_UP_1P5,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};

	struct pm8058_gpio haptics_enable = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_NO,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
		.vin_sel        = 2,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
	};

	if (machine_is_msm7x30_fluid()) {
		rc = pm8058_gpio_config(PMIC_GPIO_HAP_ENABLE, &haptics_enable);
		if (rc) {
			pr_err("%s: PMIC GPIO %d write failed\n", __func__,
				(PMIC_GPIO_HAP_ENABLE + 1));
			return rc;
		}
	}
/* support U8820 version*/
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	if (machine_is_msm7x30_fluid() || (machine_is_msm7x30_u8820()))
		sdcc_det.inv_int_pol = 1;

	rc = pm8058_gpio_config(PMIC_GPIO_SD_DET - 1, &sdcc_det);
	if (rc) {
		pr_err("%s PMIC_GPIO_SD_DET config failed\n", __func__);
		return rc;
	}
#endif

	if (machine_is_msm7x30_fluid() || (machine_is_msm7x30_u8800()) || (machine_is_msm7x30_u8820())) {
		rc = pm8058_gpio_config(PMIC_GPIO_SDC4_EN, &sdc4_en);
		if (rc) {
			pr_err("%s PMIC_GPIO_SDC4_EN config failed\n",
								 __func__);
			return rc;
		}
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC4_EN), 1);
	}
	return 0;
}
static int pm8058_pwm_config(struct pwm_device *pwm, int ch, int on)
{
	struct pm8058_gpio pwm_gpio_config = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM_GPIO_VIN_S3,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_2,
	};
	int	rc = -EINVAL;
	int	id, mode, max_mA;

	id = mode = max_mA = 0;
	switch (ch) {
	case 0:
	case 1:
	case 2:
		if (on) {
			id = 24 + ch;
			rc = pm8058_gpio_config(id - 1, &pwm_gpio_config);
			if (rc)
				pr_err("%s: pm8058_gpio_config(%d): rc=%d\n",
				       __func__, id, rc);
		}
		break;

	case 3:
		id = PM_PWM_LED_KPD;
		mode = PM_PWM_CONF_DTEST3;
		max_mA = 200;
		break;

	case 4:
		id = PM_PWM_LED_0;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 40;
		break;

	case 5:
		id = PM_PWM_LED_2;
		mode = PM_PWM_CONF_PWM2;
		max_mA = 40;
		break;

	case 6:
		id = PM_PWM_LED_FLASH;
		mode = PM_PWM_CONF_DTEST3;
		max_mA = 200;
		break;

	default:
		break;
	}

	if (ch >= 3 && ch <= 6) {
		if (!on) {
			mode = PM_PWM_CONF_NONE;
			max_mA = 0;
		}
		rc = pm8058_pwm_config_led(pwm, id, mode, max_mA);
		if (rc)
			pr_err("%s: pm8058_pwm_config_led(ch=%d): rc=%d\n",
			       __func__, ch, rc);
	}

	return rc;
}

static int pm8058_pwm_enable(struct pwm_device *pwm, int ch, int on)
{
	int	rc;

	switch (ch) {
	case 7:
		rc = pm8058_pwm_set_dtest(pwm, on);
		if (rc)
			pr_err("%s: pwm_set_dtest(%d): rc=%d\n",
			       __func__, on, rc);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static const unsigned int fluid_keymap[] = {

	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
/* application have some problem,so set the mistake keyvalue*/
/* KEY_CHAT is PTT, KEY_SOUND is handless */
	KEY(1, 0, KEY_CHAT),
	KEY(1, 1, KEY_SOUND),
};

#ifndef CONFIG_HUAWEI_KERNEL
static const unsigned int surf_keymap[] = {
	KEY(0, 0, KEY_7),
	KEY(0, 1, KEY_DOWN),
	KEY(0, 2, KEY_UP),
	KEY(0, 3, KEY_RIGHT),
	KEY(0, 4, KEY_ENTER),
	KEY(0, 5, KEY_L),
	KEY(0, 6, KEY_BACK),
	KEY(0, 7, KEY_M),

	KEY(1, 0, KEY_LEFT),
	KEY(1, 1, KEY_SEND),
	KEY(1, 2, KEY_1),
	KEY(1, 3, KEY_4),
	KEY(1, 4, KEY_CLEAR),
	KEY(1, 5, KEY_MSDOS),
	KEY(1, 6, KEY_SPACE),
	KEY(1, 7, KEY_COMMA),

	KEY(2, 0, KEY_6),
	KEY(2, 1, KEY_5),
	KEY(2, 2, KEY_8),
	KEY(2, 3, KEY_3),
	KEY(2, 4, KEY_NUMERIC_STAR),
	KEY(2, 5, KEY_UP),
	KEY(2, 6, KEY_DOWN), /* SYN */
	KEY(2, 7, KEY_LEFTSHIFT),

	KEY(3, 0, KEY_9),
	KEY(3, 1, KEY_NUMERIC_POUND),
	KEY(3, 2, KEY_0),
	KEY(3, 3, KEY_2),
	KEY(3, 4, KEY_SLEEP),
	KEY(3, 5, KEY_F1),
	KEY(3, 6, KEY_F2),
	KEY(3, 7, KEY_F3),

	KEY(4, 0, KEY_BACK),
	KEY(4, 1, KEY_HOME),
	KEY(4, 2, KEY_MENU),
	KEY(4, 3, KEY_VOLUMEUP),
	KEY(4, 4, KEY_VOLUMEDOWN),
	KEY(4, 5, KEY_F4),
	KEY(4, 6, KEY_F5),
	KEY(4, 7, KEY_F6),

	KEY(5, 0, KEY_R),
	KEY(5, 1, KEY_T),
	KEY(5, 2, KEY_Y),
	KEY(5, 3, KEY_LEFTALT),
	KEY(5, 4, KEY_KPENTER),
	KEY(5, 5, KEY_Q),
	KEY(5, 6, KEY_W),
	KEY(5, 7, KEY_E),

	KEY(6, 0, KEY_F),
	KEY(6, 1, KEY_G),
	KEY(6, 2, KEY_H),
	KEY(6, 3, KEY_CAPSLOCK),
	KEY(6, 4, KEY_PAGEUP),
	KEY(6, 5, KEY_A),
	KEY(6, 6, KEY_S),
	KEY(6, 7, KEY_D),

	KEY(7, 0, KEY_V),
	KEY(7, 1, KEY_B),
	KEY(7, 2, KEY_N),
	KEY(7, 3, KEY_MENU), /* REVISIT - SYM */
	KEY(7, 4, KEY_PAGEDOWN),
	KEY(7, 5, KEY_Z),
	KEY(7, 6, KEY_X),
	KEY(7, 7, KEY_C),

	KEY(8, 0, KEY_P),
	KEY(8, 1, KEY_J),
	KEY(8, 2, KEY_K),
	KEY(8, 3, KEY_INSERT),
	KEY(8, 4, KEY_LINEFEED),
	KEY(8, 5, KEY_U),
	KEY(8, 6, KEY_I),
	KEY(8, 7, KEY_O),

	KEY(9, 0, KEY_4),
	KEY(9, 1, KEY_5),
	KEY(9, 2, KEY_6),
	KEY(9, 3, KEY_7),
	KEY(9, 4, KEY_8),
	KEY(9, 5, KEY_1),
	KEY(9, 6, KEY_2),
	KEY(9, 7, KEY_3),

	KEY(10, 0, KEY_F7),
	KEY(10, 1, KEY_F8),
	KEY(10, 2, KEY_F9),
	KEY(10, 3, KEY_F10),
	KEY(10, 4, KEY_FN),
	KEY(10, 5, KEY_9),
	KEY(10, 6, KEY_0),
	KEY(10, 7, KEY_DOT),

	KEY(11, 0, KEY_LEFTCTRL),
	KEY(11, 1, KEY_F11),  /* START */
	KEY(11, 2, KEY_ENTER),
	KEY(11, 3, KEY_SEARCH),
	KEY(11, 4, KEY_DELETE),
	KEY(11, 5, KEY_RIGHT),
	KEY(11, 6, KEY_LEFT),
	KEY(11, 7, KEY_RIGHTSHIFT),
};
#else
static const unsigned int surf_keymap[] = {
    KEY(0, 0, KEY_VOLUMEUP),     //big_board  4 -- 4
    KEY(0, 1, KEY_VOLUMEDOWN),    //big_board  4 -- 3
    KEY(0, 2, KEY_UP),     //big_board  4 -- 2
    KEY(0, 3, KEY_RIGHT),  //big_board  4 -- 1

    KEY(1, 0, KEY_LEFT),  //big_board  3 -- 4
    KEY(1, 1, KEY_SEND), //big_board  3 -- 3
    KEY(1, 2, KEY_DOWN),     //big_board   3 -- 2
    KEY(1, 3, KEY_4),    //big_board   3 -- 1

    KEY(2, 0, KEY_6),        //big_board   2 -- 4
    KEY(2, 1, KEY_RIGHT),     //big_board   2 -- 3
    KEY(2, 2, KEY_ENTER),     //big_board   2 -- 2
    KEY(2, 3, KEY_LEFT),     //big_board   2 -- 1

    KEY(3, 0, KEY_HOME), //big_board   1 -- 4
    KEY(3, 1, KEY_BACK), //big_board  1 -- 3
    KEY(3, 2, KEY_UP),    //big_board   1 -- 2
    KEY(3, 3, KEY_MENU),   //big_board  1 -- 1
};
#endif
static struct resource resources_keypad[] = {
	{
		.start	= PM8058_KEYPAD_IRQ(PMIC8058_IRQ_BASE),
		.end	= PM8058_KEYPAD_IRQ(PMIC8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= PM8058_KEYSTUCK_IRQ(PMIC8058_IRQ_BASE),
		.end	= PM8058_KEYSTUCK_IRQ(PMIC8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct pmic8058_keypad_data surf_keypad_data = {
	.input_name		= "surf_keypad",
	.input_phys_device	= "surf_keypad/input0",
	.num_rows		= 12,
	.num_cols		= 8,
	.rows_gpio_start	= 8,
	.cols_gpio_start	= 0,
	.keymap_size		= ARRAY_SIZE(surf_keymap),
	.keymap			= surf_keymap,
	.debounce_ms		= {8, 10},
	.scan_delay_ms		= 32,
	.row_hold_ns		= 91500,
	.wakeup			= 1,
};

static struct pmic8058_keypad_data fluid_keypad_data = {
	.input_name		= "fluid-keypad",
	.input_phys_device	= "fluid-keypad/input0",
	.num_rows		= 5,
	.num_cols		= 5,
	.rows_gpio_start	= 8,
	.cols_gpio_start	= 0,
	.keymap_size		= ARRAY_SIZE(fluid_keymap),
	.keymap			= fluid_keymap,
	.debounce_ms		= {8, 10},
	.scan_delay_ms		= 32,
	.row_hold_ns		= 91500,
	.wakeup			= 1,
};

static struct pm8058_pwm_pdata pm8058_pwm_data = {
	.config		= pm8058_pwm_config,
	.enable		= pm8058_pwm_enable,
};

/* Put sub devices with fixed location first in sub_devices array */
#define	PM8058_SUBDEV_KPD	0
#define	PM8058_SUBDEV_LED	1

static struct pm8058_gpio_platform_data pm8058_gpio_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(0),
	.irq_base	= PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, 0),
	.init		= pm8058_gpios_init,
};

static struct pm8058_gpio_platform_data pm8058_mpp_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(PM8058_GPIOS),
	.irq_base	= PM8058_MPP_IRQ(PMIC8058_IRQ_BASE, 0),
};

static struct pmic8058_led pmic8058_ffa_leds[] = {
	[0] = {
		.name		= "keyboard-backlight",
		.max_brightness = 15,
		.id		= PMIC8058_ID_LED_KB_LIGHT,
	},
};

static struct pmic8058_leds_platform_data pm8058_ffa_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_ffa_leds),
	.leds	= pmic8058_ffa_leds,
};

static struct pmic8058_led pmic8058_surf_leds[] = {
	[0] = {
		.name		= "keyboard-backlight",
		.max_brightness = 15,
		.id		= PMIC8058_ID_LED_KB_LIGHT,
	},
	[1] = {
		.name		= "voice:red",
		.max_brightness = 20,
		.id		= PMIC8058_ID_LED_0,
	},
	[2] = {
		.name		= "wlan:green",
		.max_brightness = 20,
		.id		= PMIC8058_ID_LED_2,
	},
};

static struct mfd_cell pm8058_subdevs[] = {
	{	.name = "pm8058-keypad",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(resources_keypad),
		.resources	= resources_keypad,
	},
	{	.name = "pm8058-led",
		.id		= -1,
	},
	{	.name = "pm8058-gpio",
		.id		= -1,
		.platform_data	= &pm8058_gpio_data,
		.data_size	= sizeof(pm8058_gpio_data),
	},
	{	.name = "pm8058-mpp",
		.id		= -1,
		.platform_data	= &pm8058_mpp_data,
		.data_size	= sizeof(pm8058_mpp_data),
	},
	{	.name = "pm8058-pwm",
		.id		= -1,
		.platform_data	= &pm8058_pwm_data,
		.data_size	= sizeof(pm8058_pwm_data),
	},
	{	.name = "pm8058-nfc",
		.id		= -1,
	},
};

static struct pmic8058_leds_platform_data pm8058_surf_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_surf_leds),
	.leds	= pmic8058_surf_leds,
};

static struct pm8058_platform_data pm8058_7x30_data = {
	.irq_base = PMIC8058_IRQ_BASE,

	.num_subdevs = ARRAY_SIZE(pm8058_subdevs),
	.sub_devices = pm8058_subdevs,
};
/*add one more audio_amplifier_data for the right channel*/
static struct amplifier_platform_data right_audio_amplifier_data = {
    .amplifier_on = NULL,
    .amplifier_off = NULL,
    #ifdef CONFIG_HUAWEI_KERNEL
    .amplifier_4music_on = NULL,
    #endif
};
#ifdef CONFIG_HUAWEI_FEATURE_RIGHT_TPA2028D1_AMPLIFIER
static struct i2c_board_info msm_amplifier_boardinfo[]  = {
		{
		I2C_BOARD_INFO("tpa2028d1_r", 0x58),/*audio amplifier device*/
		.platform_data = &right_audio_amplifier_data,
	}
};
#endif
static struct i2c_board_info pm8058_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("pm8058-core", 0),
		.irq = MSM_GPIO_TO_INT(PMIC_GPIO_INT),
		.platform_data = &pm8058_7x30_data,
	},
};

static struct i2c_board_info msm_camera_boardinfo[] __initdata = {
#ifdef CONFIG_MT9D112
	{
		I2C_BOARD_INFO("mt9d112", 0x78 >> 1),
	},
#endif
#ifdef CONFIG_HUAWEI_SENSOR_OV7690
	{
		I2C_BOARD_INFO("ov7690", 0x42 >> 1),
	},
#endif 


#ifdef CONFIG_HUAWEI_SENSOR_HIMAX0356
	{
		I2C_BOARD_INFO("himax0356", 0x68 >> 1),
	},
#endif 

#ifdef CONFIG_HUAWEI_SENSOR_S5K4E1GX
	{
		I2C_BOARD_INFO("s5k4e1gx", 0x30 >> 1),
	},
#endif 
#ifdef CONFIG_HUAWEI_SENSOR_OV5647_SUNNY
	{
		I2C_BOARD_INFO("ov5647_sunny", 0x6c >> 1),
	},
#endif 
#ifdef CONFIG_S5K3E2FX
	{
		I2C_BOARD_INFO("s5k3e2fx", 0x20 >> 1),
	},
#endif
#ifdef CONFIG_MT9P012
	{
		I2C_BOARD_INFO("mt9p012", 0x6C >> 1),
	},
#endif
#ifdef CONFIG_VX6953
	{
		I2C_BOARD_INFO("vx6953", 0x20),
	},
#endif
#ifdef CONFIG_SN12M0PZ
	{
		I2C_BOARD_INFO("sn12m0pz", 0x34 >> 1),
	},
#endif
#if defined(CONFIG_MT9T013) || defined(CONFIG_SENSORS_MT9T013)
	{
		I2C_BOARD_INFO("mt9t013", 0x6C),
	},
#endif

};

#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(0,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* RST */
	GPIO_CFG(1,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VCM */
	GPIO_CFG(2,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
	GPIO_CFG(3,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
	GPIO_CFG(4,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	GPIO_CFG(5,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	GPIO_CFG(6,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	GPIO_CFG(7,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	GPIO_CFG(8,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT8 */
	GPIO_CFG(9,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT9 */
	GPIO_CFG(10, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT10 */
	GPIO_CFG(11, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT11 */
	GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PCLK */
	GPIO_CFG(13, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* MCLK */
	#ifdef CONFIG_HUAWEI_CAMERA
	GPIO_CFG(31, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA), /* RESET FOR OV7690*/
	GPIO_CFG(55, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* RESET FOR 4E1*/	
	GPIO_CFG(56, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VCM FOR 4E1*/	
	GPIO_CFG(88, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* ov5647*/	
	#endif
};

static uint32_t camera_on_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(0,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* RST */
	GPIO_CFG(1,  0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), /* VCM */
	GPIO_CFG(2,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
	GPIO_CFG(3,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
	GPIO_CFG(4,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	GPIO_CFG(5,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	GPIO_CFG(6,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	GPIO_CFG(7,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	GPIO_CFG(8,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT8 */
	GPIO_CFG(9,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT9 */
	GPIO_CFG(10, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT10 */
	GPIO_CFG(11, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT11 */
	GPIO_CFG(12, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PCLK */
	GPIO_CFG(13, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* MCLK */
	#ifdef CONFIG_HUAWEI_CAMERA
	GPIO_CFG(16, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), /* SCL */
	GPIO_CFG(17, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), /* SDL */
	GPIO_CFG(31, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* RESET FOR OV7690*/
	GPIO_CFG(55, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* RESET FOR 4E1*/	
	GPIO_CFG(56, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* VCM FOR 4E1*/	
	GPIO_CFG(88, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* ov5647*/	
	#endif
};

static uint32_t camera_off_gpio_fluid_table[] = {
	/* FLUID: CAM_VGA_RST_N */
	GPIO_CFG(31, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	/* FLUID: Disable CAMIF_STANDBY */
	GPIO_CFG(143, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA)
};

static uint32_t camera_on_gpio_fluid_table[] = {
	/* FLUID: CAM_VGA_RST_N */
	GPIO_CFG(31, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	/* FLUID: Disable CAMIF_STANDBY */
	GPIO_CFG(143, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA)
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

#ifdef CONFIG_HUAWEI_CAMERA
 struct msm_camera_sensor_vreg sensor_vreg_array[] = {
    {
		.vreg_name   = "gp7",
		.mv	  = 2850,
		.always_on = 0,
	},    
    {
		.vreg_name   = "gp2",
		.mv	  = 1800,
		.always_on = 0,
	},

};
static int32_t msm_camera_vreg_config_on(
       struct msm_camera_sensor_vreg *sensor_vreg,
       uint8_t vreg_num)
{
    struct vreg *vreg_handle;
    uint8_t temp_vreg_sum;
    int32_t rc;
    
    if(sensor_vreg_array == NULL)
    {
        return 0;
    }
    
    for(temp_vreg_sum = 0; temp_vreg_sum < vreg_num;temp_vreg_sum++)
    {
        vreg_handle = vreg_get(0, sensor_vreg[temp_vreg_sum].vreg_name);
    	if (!vreg_handle) {
    		printk(KERN_ERR "vreg_handle get failed\n");
    		return -EIO;
    	}
    	rc = vreg_set_level(vreg_handle, sensor_vreg[temp_vreg_sum].mv);
    	if (rc) {
    		printk(KERN_ERR "vreg_handle set level failed\n");
    		return -EIO;
    	}
    	rc = vreg_enable(vreg_handle);
    	if (rc) {
    		printk(KERN_ERR "vreg_handle enable failed\n");
    		return -EIO;
    	}
    }
    return 0;
}

static int32_t msm_camera_vreg_config_off(
       struct msm_camera_sensor_vreg *sensor_vreg,
       uint8_t vreg_num)
{
    struct vreg *vreg_handle;
    uint8_t temp_vreg_sum;
    int32_t rc;
    
    if(sensor_vreg == NULL)
    {
        return 0;
    }

    for(temp_vreg_sum = 0; temp_vreg_sum < vreg_num;temp_vreg_sum++)
    {
        if(sensor_vreg[temp_vreg_sum].always_on)
        {
            continue;
        }
        vreg_handle = vreg_get(0, sensor_vreg[temp_vreg_sum].vreg_name);
    	if (!vreg_handle) {
    		printk(KERN_ERR "vreg_handle get failed\n");
    		return -EIO;
    	}
    	rc = vreg_disable(vreg_handle);
    	if (rc) {
    		printk(KERN_ERR "vreg_handle disable failed\n");
    		return -EIO;
    	}
    }
    return 0;
}
#endif //CONFIG_HUAWEI_CAMERA


static void config_camera_on_gpios(void)
{
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));
	if (machine_is_msm7x30_fluid())
		config_gpio_table(camera_on_gpio_fluid_table,
			ARRAY_SIZE(camera_on_gpio_fluid_table));
}

static void config_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
	if (machine_is_msm7x30_fluid())
		config_gpio_table(camera_off_gpio_fluid_table,
			ARRAY_SIZE(camera_off_gpio_fluid_table));
}

struct resource msm_camera_resources[] = {
	{
		.start	= 0xA6000000,
		.end	= 0xA6000000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VFE,
		.end	= INT_VFE,
		.flags	= IORESOURCE_IRQ,
	},
};

struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.camifpadphy = 0xAB000000,
	.ioext.camifpadsz  = 0x00000400,
	.ioext.csiphy = 0xA6100000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = INT_CSI
};

/*description camera flash*/
/*raise the brightness of the first flash, it can help the AWB caculate of camera*/
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_PWM,
	._fsrc.pwm_src.freq  = 500,/*pwm freq*/
	._fsrc.pwm_src.max_load = 300,
	._fsrc.pwm_src.low_load = 100,/*low level*/
	._fsrc.pwm_src.high_load = 300,/*high level*/
	._fsrc.pwm_src.channel = 0,/*chanel id -> gpio num 24*/
};

#ifdef CONFIG_MT9D112
static struct msm_camera_sensor_flash_data flash_mt9d112 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9d112_data = {
	.sensor_name    = "mt9d112",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9d112,
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_mt9d112 = {
	.name      = "msm_camera_mt9d112",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9d112_data,
	},
};
#endif
#ifdef CONFIG_HUAWEI_SENSOR_OV7690
static struct msm_camera_sensor_flash_data flash_ov7690 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_ov7690_data = {
	.sensor_name    = "ov7690",
	.sensor_reset   = 31,
	.sensor_vreg  = sensor_vreg_array,
    	.vreg_num     = ARRAY_SIZE(sensor_vreg_array),
	.vreg_enable_func = msm_camera_vreg_config_on,
	.vreg_disable_func = msm_camera_vreg_config_off,
	.slave_sensor = 1,
	.sensor_pwd     = 0,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.flash_data     = &flash_ov7690,
	.num_resources  = ARRAY_SIZE(msm_camera_resources)
};

static struct platform_device msm_camera_sensor_ov7690 = {
	.name      = "msm_camera_ov7690",
	.dev       = {
		.platform_data = &msm_camera_sensor_ov7690_data,
	},
};
#endif

#ifdef CONFIG_HUAWEI_SENSOR_HIMAX0356
static struct msm_camera_sensor_flash_data flash_himax0356 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_himax0356_data = {
	.sensor_name    = "himax0356",
	.sensor_reset   = 31,
	.sensor_vreg  = sensor_vreg_array,
    .vreg_num     = ARRAY_SIZE(sensor_vreg_array),
	.vreg_enable_func = msm_camera_vreg_config_on,
	.vreg_disable_func = msm_camera_vreg_config_off,
	.slave_sensor = 1,
	.sensor_pwd     = 0,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.flash_data     = &flash_himax0356,
	.num_resources  = ARRAY_SIZE(msm_camera_resources)
};

static struct platform_device msm_camera_sensor_himax0356 = {
	.name      = "msm_camera_himax0356",
	.dev       = {
		.platform_data = &msm_camera_sensor_himax0356_data,
	},
};
#endif 

#ifdef CONFIG_HUAWEI_LEDS_PMIC
static struct platform_device msm_device_pmic_leds = {
	.name   = "pmic-leds",
	.id = -1,
};
#endif



#ifdef CONFIG_HUAWEI_SENSOR_S5K4E1GX
static struct msm_camera_sensor_flash_data flash_s5k4e1gx = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k4e1gx_data = {
	.sensor_name    = "s5k4e1gx",
	.sensor_reset   = 55,
	.sensor_vreg  = sensor_vreg_array,
	.vreg_num     = ARRAY_SIZE(sensor_vreg_array),
	.vreg_enable_func = msm_camera_vreg_config_on,
	.vreg_disable_func = msm_camera_vreg_config_off,
	.slave_sensor = 0,
	.sensor_pwd     = 0,
	.vcm_pwd        = 56,
	.vcm_enable     = 1,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.flash_data     = &flash_s5k4e1gx,
	.num_resources  = ARRAY_SIZE(msm_camera_resources)
};

static struct platform_device msm_camera_sensor_s5k4e1gx = {
	.name = "msm_camera_s5k4e1gx",
	.dev = {
		.platform_data = &msm_camera_sensor_s5k4e1gx_data,
	},
};
#endif

#ifdef CONFIG_HUAWEI_SENSOR_OV5647_SUNNY
static struct msm_camera_sensor_flash_data flash_ov5647_sunny = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5647_sunny_data = {
	.sensor_name    = "ov5647_sunny",
	.sensor_reset   = 88,
	.sensor_vreg  = sensor_vreg_array,
	.vreg_num     = ARRAY_SIZE(sensor_vreg_array),
	.vreg_enable_func = msm_camera_vreg_config_on,
	.vreg_disable_func = msm_camera_vreg_config_off,
	.slave_sensor = 0,
	.sensor_pwd     = 55,
	.vcm_pwd        = 56,
	.vcm_enable     = 1,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.flash_data     = &flash_ov5647_sunny,
	.num_resources  = ARRAY_SIZE(msm_camera_resources)
};

static struct platform_device msm_camera_sensor_ov5647_sunny = {
	.name = "msm_camera_ov5647_sunny",
	.dev = {
		.platform_data = &msm_camera_sensor_ov5647_sunny_data,
	},
};
#endif


#ifdef CONFIG_S5K3E2FX
static struct msm_camera_sensor_flash_data flash_s5k3e2fx = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3e2fx_data = {
	.sensor_name    = "s5k3e2fx",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_s5k3e2fx,
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_s5k3e2fx = {
	.name      = "msm_camera_s5k3e2fx",
	.dev       = {
		.platform_data = &msm_camera_sensor_s5k3e2fx_data,
	},
};
#endif

#ifdef CONFIG_MT9P012
static struct msm_camera_sensor_flash_data flash_mt9p012 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9p012_data = {
	.sensor_name    = "mt9p012",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 1,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9p012,
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_mt9p012 = {
	.name      = "msm_camera_mt9p012",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9p012_data,
	},
};
#endif
#ifdef CONFIG_VX6953
static struct msm_camera_sensor_flash_data flash_vx6953 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};
static struct msm_camera_sensor_info msm_camera_sensor_vx6953_data = {
	.sensor_name    = "vx6953",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable		= 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_vx6953,
	.csi_if         = 1
};
static struct platform_device msm_camera_sensor_vx6953 = {
	.name  	= "msm_camera_vx6953",
	.dev   	= {
		.platform_data = &msm_camera_sensor_vx6953_data,
	},
};
#endif

#ifdef CONFIG_SN12M0PZ
static struct msm_camera_sensor_flash_data flash_sn12m0pz = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};
static struct msm_camera_sensor_info msm_camera_sensor_sn12m0pz_data = {
	.sensor_name    = "sn12m0pz",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 1,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_sn12m0pz,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_sn12m0pz = {
	.name      = "msm_camera_sn12m0pz",
	.dev       = {
		.platform_data = &msm_camera_sensor_sn12m0pz_data,
	},
};
#endif

#ifdef CONFIG_MT9T013
static struct msm_camera_sensor_flash_data flash_mt9t013 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9t013_data = {
	.sensor_name    = "mt9t013",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9t013,
	.csi_if         = 1
};

static struct platform_device msm_camera_sensor_mt9t013 = {
	.name      = "msm_camera_mt9t013",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9t013_data,
	},
};
#endif

#ifdef CONFIG_MSM_GEMINI
static struct resource msm_gemini_resources[] = {
	{
		.start  = 0xA3A00000,
		.end    = 0xA3A00000 + 0x0150 - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = INT_JPEG,
		.end    = INT_JPEG,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device msm_gemini_device = {
	.name           = "msm_gemini",
	.resource       = msm_gemini_resources,
	.num_resources  = ARRAY_SIZE(msm_gemini_resources),
};
#endif

#endif /*CONFIG_MSM_CAMERA*/
#ifdef CONFIG_HUAWEI_FEATURE_RGB_KEY_LIGHT
static struct platform_device rgb_leds_device = {
	.name   = "rgb-leds",
	.id     = 0,
};
#endif

#ifdef CONFIG_MSM7KV2_AUDIO
static uint32_t audio_pamp_gpio_config =
   //GPIO_CFG(82, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA);
   GPIO_CFG(82, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA);

static uint32_t audio_fluid_icodec_tx_config =
  GPIO_CFG(85, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA);

static int __init snddev_poweramp_gpio_init(void)
{
	int rc;

	pr_info("snddev_poweramp_gpio_init \n");
	rc = gpio_tlmm_config(audio_pamp_gpio_config, GPIO_ENABLE);
	if (rc) {
		printk(KERN_ERR
			"%s: gpio_tlmm_config(%#x)=%d\n",
			__func__, audio_pamp_gpio_config, rc);
	}
	return rc;
}

void msm_snddev_tx_route_config(void)
{
	int rc;

	pr_debug("%s()\n", __func__);
	if (machine_is_msm7x30_fluid() || (machine_is_msm7x30_u8800()) || (machine_is_msm7x30_u8820())) {
		rc = gpio_tlmm_config(audio_fluid_icodec_tx_config,
		GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, audio_fluid_icodec_tx_config, rc);
		} else
			gpio_set_value(85, 0);
	}
}

void msm_snddev_tx_route_deconfig(void)
{
	int rc;

	pr_debug("%s()\n", __func__);
	if (machine_is_msm7x30_fluid() || (machine_is_msm7x30_u8800()) || (machine_is_msm7x30_u8820())) {
		rc = gpio_tlmm_config(audio_fluid_icodec_tx_config,
		GPIO_DISABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, audio_fluid_icodec_tx_config, rc);
		}
	}
}
static struct amplifier_platform_data audio_amplifier_data = {
    .amplifier_on = NULL,
    .amplifier_off = NULL,
    #ifdef CONFIG_HUAWEI_KERNEL
    .amplifier_4music_on = NULL,
    #endif
    
};
void msm_snddev_poweramp_on(void)
{
	gpio_set_value(82, 1);	/* enable spkr poweramp */
    if(audio_amplifier_data.amplifier_on)
        audio_amplifier_data.amplifier_on();
    if(right_audio_amplifier_data.amplifier_on)
		right_audio_amplifier_data.amplifier_on();
	pr_info("%s: power on amplifier\n", __func__);

#ifdef CONFIG_HUAWEI_EVALUATE_POWER_CONSUMPTION	
    /* start calculate speaker consume */
    huawei_rpc_current_consuem_notify(EVENT_SPEAKER_STATE, SPEAKER_ON_STATE );
#endif
    
}

void msm_snddev_poweramp_off(void)
{
    if(audio_amplifier_data.amplifier_off)
        audio_amplifier_data.amplifier_off();
	if(right_audio_amplifier_data.amplifier_off)
        right_audio_amplifier_data.amplifier_off();
	gpio_set_value(82, 0);	/* disable spkr poweramp */
	pr_info("%s: power off amplifier\n", __func__);

#ifdef CONFIG_HUAWEI_EVALUATE_POWER_CONSUMPTION	
    /* stop calculate speaker consume */
    huawei_rpc_current_consuem_notify(EVENT_SPEAKER_STATE, SPEAKER_OFF_STATE );
#endif

}

#ifdef CONFIG_HUAWEI_KERNEL
void msm_snddev_poweramp_4music_on(void)
{
	gpio_set_value(82, 1);	/* enable spkr poweramp */
    if(audio_amplifier_data.amplifier_4music_on)
        audio_amplifier_data.amplifier_4music_on();
    if(right_audio_amplifier_data.amplifier_4music_on)
		right_audio_amplifier_data.amplifier_4music_on();
	pr_info("%s: power on amplifier for music\n", __func__);

#ifdef CONFIG_HUAWEI_EVALUATE_POWER_CONSUMPTION	
    /* start calculate speaker consume */
    huawei_rpc_current_consuem_notify(EVENT_SPEAKER_STATE, SPEAKER_ON_STATE );
#endif

}
#endif

static struct vreg *snddev_vreg_ncp, *snddev_vreg_gp4;

void msm_snddev_hsed_voltage_on(void)
{
	int rc;

	snddev_vreg_gp4 = vreg_get(NULL, "gp4");
	if (IS_ERR(snddev_vreg_gp4)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "gp4", PTR_ERR(snddev_vreg_gp4));
		return;
	}
	rc = vreg_enable(snddev_vreg_gp4);
	if (rc)
		pr_err("%s: vreg_enable(gp4) failed (%d)\n", __func__, rc);

	snddev_vreg_ncp = vreg_get(NULL, "ncp");
	if (IS_ERR(snddev_vreg_ncp)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "ncp", PTR_ERR(snddev_vreg_ncp));
		return;
	}
	rc = vreg_enable(snddev_vreg_ncp);
	if (rc)
		pr_err("%s: vreg_enable(ncp) failed (%d)\n", __func__, rc);
}

void msm_snddev_hsed_voltage_off(void)
{
	int rc;

	if (IS_ERR(snddev_vreg_ncp)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "ncp", PTR_ERR(snddev_vreg_ncp));
		return;
	}
	rc = vreg_disable(snddev_vreg_ncp);
	if (rc)
		pr_err("%s: vreg_disable(ncp) failed (%d)\n", __func__, rc);
	vreg_put(snddev_vreg_ncp);

	if (IS_ERR(snddev_vreg_gp4)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "gp4", PTR_ERR(snddev_vreg_gp4));
		return;
	}
	rc = vreg_disable(snddev_vreg_gp4);
	if (rc)
		pr_err("%s: vreg_disable(gp4) failed (%d)\n", __func__, rc);

	vreg_put(snddev_vreg_gp4);

}

static unsigned aux_pcm_gpio_on[] = {
	GPIO_CFG(138, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* PCM_DOUT */
	GPIO_CFG(139, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),   /* PCM_DIN  */
	GPIO_CFG(140, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* PCM_SYNC */
	GPIO_CFG(141, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* PCM_CLK  */
};

static int __init aux_pcm_gpio_init(void)
{
	int pin, rc;

	pr_info("aux_pcm_gpio_init \n");
	for (pin = 0; pin < ARRAY_SIZE(aux_pcm_gpio_on); pin++) {
		rc = gpio_tlmm_config(aux_pcm_gpio_on[pin],
					GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, aux_pcm_gpio_on[pin], rc);
		}
	}
	return rc;
}
#endif /* CONFIG_MSM7KV2_AUDIO */

static int __init buses_init(void)
{
	if (gpio_tlmm_config(GPIO_CFG(PMIC_GPIO_INT, 1, GPIO_INPUT,
				  GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE))
		pr_err("%s: gpio_tlmm_config (gpio=%d) failed\n",
		       __func__, PMIC_GPIO_INT);
/* support U8820 keypad */
	if (machine_is_msm7x30_fluid() || machine_is_msm7x30_u8800() || machine_is_msm7x30_u8820()) {
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].platform_data
			= &fluid_keypad_data;
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].data_size
			= sizeof(fluid_keypad_data);
	} else {
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].platform_data
			= &surf_keypad_data;
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].data_size
			= sizeof(surf_keypad_data);
	}

	i2c_register_board_info(6 /* I2C_SSBI ID */, pm8058_boardinfo,
				ARRAY_SIZE(pm8058_boardinfo));

	return 0;
}

static struct vreg *vreg_marimba_1;
static struct vreg *vreg_marimba_2;

static unsigned int msm_marimba_setup_power(void)
{
	int rc;

	rc = vreg_enable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto out;
	}
	rc = vreg_enable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto out;
	}

out:
	return rc;
};

static void msm_marimba_shutdown_power(void)
{
	int rc;

	rc = vreg_disable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	rc = vreg_disable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
};

static int fm_radio_setup(struct marimba_fm_platform_data *pdata)
{
	int rc;
	uint32_t irqcfg;
	const char *id = "FMPW";

	pdata->vreg_s2 = vreg_get(NULL, "s2");
	if (IS_ERR(pdata->vreg_s2)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(pdata->vreg_s2));
		return -1;
	}

	rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 1300);
	if (rc < 0) {
		printk(KERN_ERR "%s: voltage level vote failed (%d)\n",
			__func__, rc);
		return rc;
	}

	rc = vreg_enable(pdata->vreg_s2);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		return rc;
	}

	rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_ON);
	if (rc < 0) {
		printk(KERN_ERR "%s: clock vote failed (%d)\n",
			__func__, rc);
		goto fm_clock_vote_fail;
	}
	irqcfg = GPIO_CFG(147, 0, GPIO_INPUT, GPIO_NO_PULL,
					GPIO_2MA);
	rc = gpio_tlmm_config(irqcfg, GPIO_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, irqcfg, rc);
		rc = -EIO;
		goto fm_gpio_config_fail;

	}
	return 0;
fm_gpio_config_fail:
	pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
				  PMAPP_CLOCK_VOTE_OFF);
fm_clock_vote_fail:
	vreg_disable(pdata->vreg_s2);
	return rc;

};

static void fm_radio_shutdown(struct marimba_fm_platform_data *pdata)
{
	int rc;
	const char *id = "FMPW";
	uint32_t irqcfg = GPIO_CFG(147, 0, GPIO_INPUT, GPIO_PULL_UP,
					GPIO_2MA);
	rc = gpio_tlmm_config(irqcfg, GPIO_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, irqcfg, rc);
	}
	rc = vreg_disable(pdata->vreg_s2);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_OFF);
	if (rc < 0)
		printk(KERN_ERR "%s: clock_vote return val: %d \n",
						__func__, rc);
	rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 0);
	if (rc < 0)
		printk(KERN_ERR "%s: vreg level vote return val: %d \n",
						__func__, rc);
}

static struct marimba_fm_platform_data marimba_fm_pdata = {
	.fm_setup =  fm_radio_setup,
	.fm_shutdown = fm_radio_shutdown,
	.irq = MSM_GPIO_TO_INT(147),
	.vreg_s2 = NULL,
	.vreg_xo_out = NULL,
};


/* Slave id address for FM/CDC/QMEMBIST
 * Values can be programmed using Marimba slave id 0
 * should there be a conflict with other I2C devices
 * */
#define MARIMBA_SLAVE_ID_FM_ADDR	0x2A
#define MARIMBA_SLAVE_ID_CDC_ADDR	0x77
#define MARIMBA_SLAVE_ID_QMEMBIST_ADDR	0X66

static const char *tsadc_id = "MADC";
static const char *vregs_tsadc_name[] = {
	"gp12",
	"s2",
};
static struct vreg *vregs_tsadc[ARRAY_SIZE(vregs_tsadc_name)];

static int marimba_tsadc_power(int vreg_on)
{
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(vregs_tsadc_name); i++) {
		if (!vregs_tsadc[i]) {
			printk(KERN_ERR "%s: vreg_get %s failed (%d)\n",
				__func__, vregs_tsadc_name[i], rc);
			goto vreg_fail;
		}

		rc = vreg_on ? vreg_enable(vregs_tsadc[i]) :
			  vreg_disable(vregs_tsadc[i]);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg %s %s failed (%d)\n",
				__func__, vregs_tsadc_name[i],
			       vreg_on ? "enable" : "disable", rc);
			goto vreg_fail;
		}
	}
	/* vote for D0 buffer */
	rc = pmapp_clock_vote(tsadc_id, PMAPP_CLOCK_ID_DO,
		vreg_on ? PMAPP_CLOCK_VOTE_ON : PMAPP_CLOCK_VOTE_OFF);
	if (rc)	{
		printk(KERN_ERR "%s: unable to %svote for d0 clk\n",
			__func__, vreg_on ? "" : "de-");
		goto do_vote_fail;
	}

	msleep(5); /* ensure power is stable */

	return 0;

do_vote_fail:
vreg_fail:
	while (i)
		vreg_disable(vregs_tsadc[--i]);
	return rc;
}

static int marimba_tsadc_vote(int vote_on)
{
	int rc, level;

	level = vote_on ? 1300 : 0;

	rc = pmapp_vreg_level_vote(tsadc_id, PMAPP_VREG_S2, level);
	if (rc < 0)
		printk(KERN_ERR "%s: vreg level %s failed (%d)\n",
			__func__, vote_on ? "on" : "off", rc);

	return rc;
}

static int marimba_tsadc_init(void)
{
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(vregs_tsadc_name); i++) {
		vregs_tsadc[i] = vreg_get(NULL, vregs_tsadc_name[i]);
		if (IS_ERR(vregs_tsadc[i])) {
			printk(KERN_ERR "%s: vreg get %s failed (%ld)\n",
			       __func__, vregs_tsadc_name[i],
			       PTR_ERR(vregs_tsadc[i]));
			rc = PTR_ERR(vregs_tsadc[i]);
			goto vreg_get_fail;
		}
	}

	return rc;

vreg_get_fail:
	while (i)
		vreg_put(vregs_tsadc[--i]);
	return rc;
}

static int marimba_tsadc_exit(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(vregs_tsadc_name); i++) {
		if (vregs_tsadc[i])
			vreg_put(vregs_tsadc[i]);
	}

	rc = pmapp_vreg_level_vote(tsadc_id, PMAPP_VREG_S2, 0);
	if (rc < 0)
		printk(KERN_ERR "%s: vreg level off failed (%d)\n",
			__func__, rc);

	return rc;
}


static struct msm_ts_platform_data msm_ts_data = {
	.min_x          = 0,
	.max_x          = 4096,
	.min_y          = 0,
	.max_y          = 4096,
	.min_press      = 0,
	.max_press      = 255,
	.inv_x          = 4096,
	.inv_y          = 4096,
};

static struct marimba_tsadc_platform_data marimba_tsadc_pdata = {
	.marimba_tsadc_power =  marimba_tsadc_power,
	.init		     =  marimba_tsadc_init,
	.exit		     =  marimba_tsadc_exit,
	.level_vote	     =  marimba_tsadc_vote,
	.tsadc_prechg_en = true,
	.setup = {
		.pen_irq_en	=	true,
		.tsadc_en	=	true,
	},
	.params2 = {
		.input_clk_khz		=	2400,
		.sample_prd		=	TSADC_CLK_3,
	},
	.params3 = {
		.prechg_time_nsecs	=	6400,
		.stable_time_nsecs	=	6400,
		.tsadc_test_mode	=	0,
	},
	.tssc_data = &msm_ts_data,
};

static struct vreg *vreg_codec_s4;
static int msm_marimba_codec_power(int vreg_on)
{
	int rc = 0;

	if (!vreg_codec_s4) {

		vreg_codec_s4 = vreg_get(NULL, "s4");

		if (IS_ERR(vreg_codec_s4)) {
			printk(KERN_ERR "%s: vreg_get() failed (%ld)\n",
				__func__, PTR_ERR(vreg_codec_s4));
			rc = PTR_ERR(vreg_codec_s4);
			goto  vreg_codec_s4_fail;
		}
	}

	if (vreg_on) {
		rc = vreg_enable(vreg_codec_s4);
		if (rc)
			printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto vreg_codec_s4_fail;
	} else {
		rc = vreg_disable(vreg_codec_s4);
		if (rc)
			printk(KERN_ERR "%s: vreg_disable() = %d \n",
					__func__, rc);
		goto vreg_codec_s4_fail;
	}

vreg_codec_s4_fail:
	return rc;
}

static struct marimba_codec_platform_data mariba_codec_pdata = {
	.marimba_codec_power =  msm_marimba_codec_power,
};

static struct marimba_platform_data marimba_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_FM]       = MARIMBA_SLAVE_ID_FM_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_CDC]	     = MARIMBA_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = MARIMBA_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_marimba_setup_power,
	.marimba_shutdown = msm_marimba_shutdown_power,
	.fm = &marimba_fm_pdata,
	.codec = &mariba_codec_pdata,
};

static void __init msm7x30_init_marimba(void)
{
	vreg_marimba_1 = vreg_get(NULL, "s2");
	if (IS_ERR(vreg_marimba_1)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_marimba_1));
		return;
	}
	vreg_marimba_2 = vreg_get(NULL, "gp16");
	if (IS_ERR(vreg_marimba_1)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_marimba_1));
		return;
	}
}

#ifdef CONFIG_MSM7KV2_AUDIO
static struct resource msm_aictl_resources[] = {
	{
		.name = "aictl",
		.start = 0xa5000100,
		.end = 0xa5000100,
		.flags = IORESOURCE_MEM,
	}
};

static struct resource msm_mi2s_resources[] = {
	{
		.name = "hdmi",
		.start = 0xac900000,
		.end = 0xac900038,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "codec_rx",
		.start = 0xac940040,
		.end = 0xac940078,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "codec_tx",
		.start = 0xac980080,
		.end = 0xac9800B8,
		.flags = IORESOURCE_MEM,
	}

};

static struct msm_lpa_platform_data lpa_pdata = {
	.obuf_hlb_size = 0x2BFF8,
	.dsp_proc_id = 0,
	.app_proc_id = 2,
	.nosb_config = {
		.llb_min_addr = 0,
		.llb_max_addr = 0x3ff8,
		.sb_min_addr = 0,
		.sb_max_addr = 0,
	},
	.sb_config = {
		.llb_min_addr = 0,
		.llb_max_addr = 0x37f8,
		.sb_min_addr = 0x3800,
		.sb_max_addr = 0x3ff8,
	}
};

static struct resource msm_lpa_resources[] = {
	{
		.name = "lpa",
		.start = 0xa5000000,
		.end = 0xa50000a0,
		.flags = IORESOURCE_MEM,
	}
};

static struct resource msm_aux_pcm_resources[] = {

	{
		.name = "aux_codec_reg_addr",
		.start = 0xac9c00c0,
		.end = 0xac9c00c8,
		.flags = IORESOURCE_MEM,
	},
	{
		.name   = "aux_pcm_dout",
		.start  = 138,
		.end    = 138,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_din",
		.start  = 139,
		.end    = 139,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_syncout",
		.start  = 140,
		.end    = 140,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_clkin_a",
		.start  = 141,
		.end    = 141,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_aux_pcm_device = {
	.name   = "msm_aux_pcm",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_aux_pcm_resources),
	.resource       = msm_aux_pcm_resources,
};

struct platform_device msm_aictl_device = {
	.name = "audio_interct",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_aictl_resources),
	.resource = msm_aictl_resources,
};

struct platform_device msm_mi2s_device = {
	.name = "mi2s",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_mi2s_resources),
	.resource = msm_mi2s_resources,
};

struct platform_device msm_lpa_device = {
	.name = "lpa",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_lpa_resources),
	.resource = msm_lpa_resources,
	.dev		= {
		.platform_data = &lpa_pdata,
	},
};
#endif /* CONFIG_MSM7KV2_AUDIO */

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
 #define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
 #define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)

static unsigned int dec_concurrency_table[] = {
	/* Audio LP */
	0,
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_MODE_LP)|
	(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 1 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),

	 /* Concurrency 2 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 3 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 4 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 5 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 6 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

#define DEC_INSTANCE(max_instance_same, max_instance_diff) { \
	.max_instances_same_dec = max_instance_same, \
	.max_instances_diff_dec = max_instance_diff}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  /* AudPlay4BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY3TASK", 16, 3, 11),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY2TASK", 15, 2, 11),  /* AudPlay2BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY1TASK", 14, 1, 11),  /* AudPlay1BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), /* AudPlay0BitStreamCtrlQueue */
};

static struct dec_instance_table dec_instance_list[][MSM_MAX_DEC_CNT] = {
	/* Non Turbo Mode */
	{
		DEC_INSTANCE(4, 3), /* WAV */
		DEC_INSTANCE(4, 3), /* ADPCM */
		DEC_INSTANCE(4, 2), /* MP3 */
		DEC_INSTANCE(0, 0), /* Real Audio */
		DEC_INSTANCE(4, 2), /* WMA */
		DEC_INSTANCE(3, 2), /* AAC */
		DEC_INSTANCE(0, 0), /* Reserved */
		DEC_INSTANCE(0, 0), /* MIDI */
		DEC_INSTANCE(4, 3), /* YADPCM */
		DEC_INSTANCE(4, 3), /* QCELP */
		DEC_INSTANCE(4, 3), /* AMRNB */
		DEC_INSTANCE(1, 1), /* AMRWB/WB+ */
		DEC_INSTANCE(4, 3), /* EVRC */
		DEC_INSTANCE(1, 1), /* WMAPRO */
	},
	/* Turbo Mode */
	{
		DEC_INSTANCE(4, 3), /* WAV */
		DEC_INSTANCE(4, 3), /* ADPCM */
		DEC_INSTANCE(4, 3), /* MP3 */
		DEC_INSTANCE(0, 0), /* Real Audio */
		DEC_INSTANCE(4, 3), /* WMA */
		DEC_INSTANCE(4, 3), /* AAC */
		DEC_INSTANCE(0, 0), /* Reserved */
		DEC_INSTANCE(0, 0), /* MIDI */
		DEC_INSTANCE(4, 3), /* YADPCM */
		DEC_INSTANCE(4, 3), /* QCELP */
		DEC_INSTANCE(4, 3), /* AMRNB */
		DEC_INSTANCE(2, 3), /* AMRWB/WB+ */
		DEC_INSTANCE(4, 3), /* EVRC */
		DEC_INSTANCE(1, 2), /* WMAPRO */
	},
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
	.dec_instance_list = &dec_instance_list[0][0],
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_adspdec_database
	},
};

	/* removed several lines */

#ifdef CONFIG_USB_FUNCTION
static struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
	.nluns          = 0x02,
	.buf_size       = 16384,
	.vendor         = "GOOGLE",
	.product        = "Mass storage",
	.release        = 0xffff,
};

static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &usb_mass_storage_pdata,
	},
};
#endif
#ifdef CONFIG_USB_ANDROID
/* dynamic composition */
static struct usb_composition usb_func_composition[] = {
	{
		/* MSC */
		.product_id         = 0xF000,
		.functions	    = 0x02,
		.adb_product_id     = 0x9015,
		.adb_functions	    = 0x12
	},
#ifdef CONFIG_USB_F_SERIAL
	{
		/* MODEM */
		.product_id         = 0xF00B,
		.functions	    = 0x06,
		.adb_product_id     = 0x901E,
		.adb_functions	    = 0x16,
	},
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	{
		/* DIAG */
		.product_id         = 0x900E,
		.functions	    = 0x04,
		.adb_product_id     = 0x901D,
		.adb_functions	    = 0x14,
	},
#endif
#if defined(CONFIG_USB_ANDROID_DIAG) && defined(CONFIG_USB_F_SERIAL)
	{
		/* DIAG + MODEM */
		.product_id         = 0x9004,
		.functions	    = 0x64,
		.adb_product_id     = 0x901F,
		.adb_functions	    = 0x0614,
	},
	{
		/* DIAG + MODEM + NMEA*/
		.product_id         = 0x9016,
		.functions	    = 0x764,
		.adb_product_id     = 0x9020,
		.adb_functions	    = 0x7614,
	},
	{
		/* DIAG + MODEM + NMEA + MSC */
		.product_id         = 0x9017,
		.functions	    = 0x2764,
		.adb_product_id     = 0x9018,
		.adb_functions	    = 0x27614,
	},
#endif

#ifdef CONFIG_USB_AUTO_INSTALL
    /* new requirement: usb tethering */
    {
        /* MSC, WLAN */
        .product_id         = PID_WLAN,
        .functions      = 0xA,
        .adb_product_id     = PID_WLAN,
        .adb_functions      = 0xA,
    },

/* add new pid config for google */
/* < google_usb_drv */	
	{
		/* MSC, MSC+ADB */
		.product_id         = PID_GOOGLE_MS,
		.functions	    = 0x2,
		.adb_product_id     = PID_GOOGLE,
		.adb_functions	    = 0x12,
	},
/* google_usb_drv > */	
	{
		/* CDROM */
		.product_id         = PID_ONLY_CDROM,
		.functions	    = 0x2,
		.adb_product_id     = PID_ONLY_CDROM,
		.adb_functions	    = 0x2,
	},
	{
		/* GENERIC_MODEM + ACM_MODEM + MSC + ADB + DIAG */
		.product_id         = PID_NORMAL,
		.functions	    = 0x276,
		.adb_product_id     = PID_NORMAL,
		.adb_functions	    = 0x41276,
	},
	{
		/* UDISK */
		.product_id         = PID_UDISK,
		.functions	    = 0x2,
		.adb_product_id     = PID_UDISK,
		.adb_functions	    = 0x2,
	},
	{
		/* GENERIC_MODEM + ACM_MODEM + MSC + ADB + DIAG */
		.product_id         = PID_AUTH,
		.functions	    = 0x41276,
		.adb_product_id     = PID_AUTH,
		.adb_functions	    = 0x41276,
	},

#endif

#ifdef CONFIG_USB_ANDROID_CDC_ECM
	{
		/* MSC + CDC-ECM */
		.product_id         = 0x9014,
		.functions	    = 0x82,
		.adb_product_id     = 0x9023,
		.adb_functions	    = 0x812,
	},
#endif
#ifdef CONFIG_USB_ANDROID_RMNET
	{
		/* DIAG + RMNET */
		.product_id         = 0x9021,
		.functions	    = 0x94,
		.adb_product_id     = 0x9022,
		.adb_functions	    = 0x914,
	},
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	{
		/* RNDIS */
		.product_id         = 0xF00E,
		.functions	    = 0xA,
		.adb_product_id     = 0x9024,
		.adb_functions	    = 0x1A,
	},
#endif
};
static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "GOOGLE",
	.product	= "Mass Storage",
	.release	= 0xFFFF,
};
static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &mass_storage_pdata,
	},
};
#ifdef CONFIG_USB_AUTO_INSTALL
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= HUAWEI_VID,
/* < google_usb_drv */	
//	.vendor_id	= 0x18D1, /* 0x05C6, */
/* google_usb_drv > */	
	.version	= 0x0100,
	.compositions   = usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
/* add new pid config for google */
	.product_name	= "Huawei HSUSB Device",
	.manufacturer_name = "Huawei Incorporated",
	.nluns = 2,
};
#else
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x12d1,
	.version	= 0x0100,
	.compositions   = usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
	.product_name	= "Qualcomm HSUSB Device",
	.manufacturer_name = "Qualcomm Incorporated",
    #ifndef CONFIG_HUAWEI_KERNEL
	.nluns = 1,
	#else
    .nluns = 2,
    #endif
};
#endif

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};
#endif
/*remove the statements about m33c01*/

/* kernel29 -> kernel32 driver modify*/
/* delete some lines*/
lcd_panel_type lcd_panel_probe(void)
{
    lcd_panel_type hw_lcd_panel = LCD_NONE;
    if((machine_is_msm7x30_u8800()) || (machine_is_msm7x30_u8820()))
	{
		switch (lcd_id)
		{
			case 0:  
				hw_lcd_panel = LCD_NT35582_BYD_WVGA;
			break;
		 	case 1:  
				hw_lcd_panel = LCD_NT35582_TRULY_WVGA;
			break;
			/* NT35510 alpha_si is MDDI type2 LCD */
			case 2:  
				hw_lcd_panel = LCD_NT35510_ALPHA_SI_WVGA;
			break;

			default : 
				hw_lcd_panel = LCD_NONE;
			break;					  
		}
	}
    printk(KERN_ERR "lcd_panel:*********hw_lcd_panel == %d;***************\n", hw_lcd_panel);
    return hw_lcd_panel;

}


char *get_lcd_panel_name(void)
{
    lcd_panel_type hw_lcd_panel = LCD_NONE;
    char *pname = NULL;
    
    hw_lcd_panel = lcd_panel_probe();
    
    switch (hw_lcd_panel)
    {
     case LCD_S6D74A0_SAMSUNG_HVGA:
            pname = "SAMSUNG S6D74A0";
            break;
            
        case LCD_ILI9325_INNOLUX_QVGA:
            pname = "INNOLUX ILI9325";
            break;

        case LCD_ILI9325_BYD_QVGA:
            pname = "BYD ILI9325";
            break;

        case LCD_ILI9325_WINTEK_QVGA:
            pname = "WINTEK ILI9325";
            break;

        case LCD_SPFD5408B_KGM_QVGA:
            pname = "KGM SPFD5408B";
            break;

        case LCD_HX8357A_BYD_QVGA:
            pname = "BYD HX8357A";
            break;

        case LCD_HX8368A_SEIKO_QVGA:
            pname = "SEIKO HX8368A";
            break;
			
        case LCD_HX8347D_TRULY_QVGA:
            pname = "TRULY HX8347D";
            break;
			
        case LCD_ILI9325C_WINTEK_QVGA:
            pname = "WINTEK ILI9325C";
            break;
						
        case LCD_NT35582_BYD_WVGA:
            pname = "BYD NT35582";
            break;
            
        case LCD_NT35582_TRULY_WVGA:
            pname = "TRULY NT35582";
            break;
    	case LCD_NT35510_ALPHA_SI_WVGA:
		    pname = "TRULY NT35510";
            break;

            
        default:
            pname = "UNKNOWN LCD";
            break;
    }

    return pname;
}


/* removed several lines */

static struct i2c_board_info msm_i2c_board_info[] = {
#ifdef CONFIG_HUAWEI_FEATURE_TPA2028D1_AMPLIFIER
    {   
		I2C_BOARD_INFO("tpa2028d1", 0x58),  
        .platform_data = &audio_amplifier_data,
	},
#endif
	#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ST_LSM303DLH
	{
		I2C_BOARD_INFO("st303_gs", 0x32 >> 1),                
		//.irq = MSM_GPIO_TO_INT() 
	},
	{
		I2C_BOARD_INFO("st303_compass", 0x3e >> 1),/* actual i2c address is 0x3c    */             
		//.irq = MSM_GPIO_TO_INT() 
	},
	#endif
#ifdef CONFIG_HUAWEI_FEATURE_PROXIMITY_EVERLIGHT_APS_12D
	{   
		I2C_BOARD_INFO("aps-12d", 0x88 >> 1),  
	},
#endif
#ifdef CONFIG_HUAWEI_FEATURE_RMI_TOUCH
	{   
		I2C_BOARD_INFO("Synaptics_rmi", 0x70),
		.irq = MSM_GPIO_TO_INT(ATMEL_RMI_TS_IRQ),
	},
#endif
#ifdef CONFIG_HUAWEI_FEATURE_AT42QT_TS
	{   
		I2C_BOARD_INFO("atmel-rmi-ts", 0x4a),  
/* delete some lines*/
        .irq = MSM_GPIO_TO_INT(ATMEL_RMI_TS_IRQ),
	},
#endif
/* removed several lines */
};

static struct i2c_board_info msm_marimba_board_info[] = {
	{
		I2C_BOARD_INFO("marimba", 0xc),
		.platform_data = &marimba_pdata,
	}
};

#ifdef CONFIG_USB_FUNCTION
static struct usb_function_map usb_functions_map[] = {
	{"diag", 0},
	{"adb", 1},
	{"modem", 2},
	{"nmea", 3},
	{"mass_storage", 4},
	{"ethernet", 5},
};

static struct usb_composition usb_func_composition[] = {
	{
		.product_id         = 0x9012,
		.functions	    = 0x5, /* 0101 */
	},

	{
		.product_id         = 0x9013,
		.functions	    = 0x15, /* 10101 */
	},

	{
		.product_id         = 0x9014,
		.functions	    = 0x30, /* 110000 */
	},

	{
		.product_id         = 0x9016,
		.functions	    = 0xD, /* 01101 */
	},

	{
		.product_id         = 0x9017,
		.functions	    = 0x1D, /* 11101 */
	},

	{
		.product_id         = 0xF000,
		.functions	    = 0x10, /* 10000 */
	},

	{
		.product_id         = 0xF009,
		.functions	    = 0x20, /* 100000 */
	},

	{
		.product_id         = 0x9018,
		.functions	    = 0x1F, /* 011111 */
	},

};
static struct msm_hsusb_platform_data msm_hsusb_pdata = {
	.version	= 0x0100,
	.phy_info	= USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM,
	.vendor_id	= 0x12d1,
	.product_name	= "Qualcomm HSUSB Device",
	.serial_number	= "1234567890ABCDEF",
	.manufacturer_name
			= "Qualcomm Incorporated",
	.compositions	= usb_func_composition,
	.num_compositions
			= ARRAY_SIZE(usb_func_composition),
	.function_map	= usb_functions_map,
	.num_functions	= ARRAY_SIZE(usb_functions_map),
	.core_clk	= 1,
};
#endif

static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_device = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 8594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].residency = 23740,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 4594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].residency = 23740,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].suspend_enabled = 0,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].latency = 500,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].residency = 6000,

	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].suspend_enabled
		= 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].idle_enabled = 0,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 443,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].residency = 1098,

	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency = 2,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].residency = 0,
};

static struct resource qsd_spi_resources[] = {
	{
		.name   = "spi_irq_in",
		.start	= INT_SPI_INPUT,
		.end	= INT_SPI_INPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_out",
		.start	= INT_SPI_OUTPUT,
		.end	= INT_SPI_OUTPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_err",
		.start	= INT_SPI_ERROR,
		.end	= INT_SPI_ERROR,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_base",
		.start	= 0xA8000000,
		.end	= 0xA8000000 + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name   = "spidm_channels",
		.flags  = IORESOURCE_DMA,
	},
	{
		.name   = "spidm_crci",
		.flags  = IORESOURCE_DMA,
	},
};

#define AMDH0_BASE_PHYS		0xAC200000
#define ADMH0_GP_CTL		(ct_adm_base + 0x3D8)
static int msm_qsd_spi_dma_config(void)
{
	void __iomem *ct_adm_base = 0;
	u32 spi_mux = 0;
	int ret = 0;

	ct_adm_base = ioremap(AMDH0_BASE_PHYS, PAGE_SIZE);
	if (!ct_adm_base) {
		pr_err("%s: Could not remap %x\n", __func__, AMDH0_BASE_PHYS);
		return -ENOMEM;
	}

	spi_mux = (ioread32(ADMH0_GP_CTL) & (0x3 << 12)) >> 12;

	qsd_spi_resources[4].start  = DMOV_USB_CHAN;
	qsd_spi_resources[4].end    = DMOV_TSIF_CHAN;

	switch (spi_mux) {
	case (1):
		qsd_spi_resources[5].start  = DMOV_HSUART1_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART1_TX_CRCI;
		break;
	case (2):
		qsd_spi_resources[5].start  = DMOV_HSUART2_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART2_TX_CRCI;
		break;
	case (3):
		qsd_spi_resources[5].start  = DMOV_CE_OUT_CRCI;
		qsd_spi_resources[5].end    = DMOV_CE_IN_CRCI;
		break;
	default:
		ret = -ENOENT;
	}

	iounmap(ct_adm_base);

	return ret;
}

static struct platform_device qsd_device_spi = {
	.name		= "spi_qsd",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qsd_spi_resources),
	.resource	= qsd_spi_resources,
};

static struct msm_gpio qsd_spi_gpio_config_data[] = {
	{ GPIO_CFG(45, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA), "spi_clk" },
	{ GPIO_CFG(46, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_8MA), "spi_mosi" },
	{ GPIO_CFG(48, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA), "spi_miso" },
};

static int msm_qsd_spi_gpio_config(void)
{
	return msm_gpios_request_enable(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static void msm_qsd_spi_gpio_release(void)
{
	msm_gpios_disable_free(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static struct msm_spi_platform_data qsd_spi_pdata = {
	.max_clock_speed = 26000000,
	.clk_name = "spi_clk",
	.pclk_name = "spi_pclk",
	.gpio_config  = msm_qsd_spi_gpio_config,
	.gpio_release = msm_qsd_spi_gpio_release,
	.dma_config = msm_qsd_spi_dma_config,
};

static void __init msm_qsd_spi_init(void)
{
	qsd_device_spi.dev.platform_data = &qsd_spi_pdata;
}

#ifdef CONFIG_USB_EHCI_MSM
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
	int rc;
	static int vbus_is_on;
	struct pm8058_gpio usb_vbus = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_NO,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.vin_sel        = 2,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};

	/* If VBUS is already on (or off), do nothing. */
	if (unlikely(on == vbus_is_on))
		return;

	if (on) {
		rc = pm8058_gpio_config(36, &usb_vbus);
		if (rc) {
			pr_err("%s PMIC GPIO 36 write failed\n", __func__);
			return;
		}
	} else
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(36), 0);

	vbus_is_on = on;
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info   = (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
	.power_budget   = 180,
};
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
#ifndef CONFIG_USB_EHCI_MSM
static int ldo_on;
static struct vreg *usb_vreg;
static int msm_pmic_enable_ldo(int enable)
{
	if (ldo_on == enable)
		return 0;

	ldo_on = enable;

	if (enable)
		return vreg_enable(usb_vreg);
	else
		return vreg_disable(usb_vreg);
}

static int msm_pmic_notify_init(void)
{
	usb_vreg = vreg_get(NULL, "usb");
	if (IS_ERR(usb_vreg)) {
		pr_err("%s: usb vreg get failed\n", __func__);
		vreg_put(usb_vreg);
		return PTR_ERR(usb_vreg);
	}

	return 0;
}

static void msm_pmic_notify_deinit(void)
{
	msm_pmic_enable_ldo(0);
	vreg_put(usb_vreg);
}
#endif
static struct msm_otg_platform_data msm_otg_pdata = {
	.rpc_connect	= hsusb_rpc_connect,

#ifndef CONFIG_USB_EHCI_MSM
	/* vbus notification through pmic call backs */
	.pmic_notif_init         = msm_pmic_notify_init,
	.pmic_notif_deinit       = msm_pmic_notify_deinit,
	.pmic_enable_ldo         = msm_pmic_enable_ldo,
	.pmic_vbus_irq	= 1,
#else
	.vbus_power = msm_hsusb_vbus_power,
#endif
	.core_clk	= 1,
	.pemp_level     = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset  = CDR_AUTO_RESET_DISABLE,
	.drv_ampl       = HS_DRV_AMPLITUDE_DEFAULT,
};

#ifdef CONFIG_USB_GADGET
static struct msm_hsusb_gadget_platform_data msm_gadget_pdata;
#endif
#endif

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

/* removed several lines */

static struct msm_gpio dtv_panel_gpios[] = {
	{ GPIO_CFG(18, 0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_4MA), "hdmi_int" },
	{ GPIO_CFG(120, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "wca_mclk" },
	{ GPIO_CFG(121, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "wca_sd0" },
	{ GPIO_CFG(122, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "wca_sd1" },
	{ GPIO_CFG(123, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "wca_sd2" },
	{ GPIO_CFG(124, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_8MA), "dtv_pclk" },
	{ GPIO_CFG(125, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_en" },
	{ GPIO_CFG(126, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_vsync" },
	{ GPIO_CFG(127, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_hsync" },
	{ GPIO_CFG(128, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_data0" },
	{ GPIO_CFG(129, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_data1" },
	{ GPIO_CFG(130, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_data2" },
	{ GPIO_CFG(131, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_data3" },
	{ GPIO_CFG(132, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_data4" },
	{ GPIO_CFG(160, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_data5" },
	{ GPIO_CFG(161, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_data6" },
	{ GPIO_CFG(162, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_data7" },
	{ GPIO_CFG(163, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_data8" },
	{ GPIO_CFG(164, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_data9" },
	{ GPIO_CFG(165, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat10" },
	{ GPIO_CFG(166, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat11" },
	{ GPIO_CFG(167, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat12" },
	{ GPIO_CFG(168, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat13" },
	{ GPIO_CFG(169, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat14" },
	{ GPIO_CFG(170, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat15" },
	{ GPIO_CFG(171, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat16" },
	{ GPIO_CFG(172, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat17" },
	{ GPIO_CFG(173, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat18" },
	{ GPIO_CFG(174, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat19" },
	{ GPIO_CFG(175, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat20" },
	{ GPIO_CFG(176, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat21" },
	{ GPIO_CFG(177, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat22" },
	{ GPIO_CFG(178, 1, GPIO_OUTPUT,  GPIO_NO_PULL, GPIO_4MA), "dtv_dat23" },
};


#ifdef HDMI_RESET
static unsigned dtv_reset_gpio =
	GPIO_CFG(37, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA);
#endif

static int dtv_panel_power(int on)
{
	int flag_on = !!on;
	static int dtv_power_save_on;
	struct vreg *vreg_ldo17, *vreg_ldo8;
	int rc;

	if (dtv_power_save_on == flag_on)
		return 0;

	dtv_power_save_on = flag_on;

#ifdef HDMI_RESET
	if (on) {
		/* reset Toshiba WeGA chip -- toggle reset pin -- gpio_180 */
		rc = gpio_tlmm_config(dtv_reset_gpio, GPIO_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, dtv_reset_gpio, rc);
			return rc;
		}

		gpio_set_value(37, 0);	/* bring reset line low to hold reset*/
	}
#endif

	if (on) {
		rc = msm_gpios_enable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio enable failed: %d\n",
				__func__, rc);
			return rc;
		}
	} else {
		rc = msm_gpios_disable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio disable failed: %d\n",
				__func__, rc);
			return rc;
		}
	}

	vreg_ldo8 = vreg_get(NULL, "gp7");

	if (IS_ERR(vreg_ldo8)) {
		rc = PTR_ERR(vreg_ldo8);
		pr_err("%s:  vreg17 get failed (%d)\n",
			__func__, rc);
		return rc;
	}

	rc = vreg_set_level(vreg_ldo8, 1800);
	if (rc) {
		pr_err("%s: vreg LDO18 set level failed (%d)\n",
			__func__, rc);
		return rc;
	}

	if (on)
		rc = vreg_enable(vreg_ldo8);
	else
		rc = vreg_disable(vreg_ldo8);

	if (rc) {
		pr_err("%s: LDO8 vreg enable failed (%d)\n",
			__func__, rc);
		return rc;
	}

	mdelay(5);		/* ensure power is stable */

	/*  -- LDO17 for HDMI */
	vreg_ldo17 = vreg_get(NULL, "gp11");

	if (IS_ERR(vreg_ldo17)) {
		rc = PTR_ERR(vreg_ldo17);
		pr_err("%s:  vreg17 get failed (%d)\n",
			__func__, rc);
		return rc;
	}

	rc = vreg_set_level(vreg_ldo17, 2600);
	if (rc) {
		pr_err("%s: vreg LDO17 set level failed (%d)\n",
			__func__, rc);
		return rc;
	}

	if (on)
		rc = vreg_enable(vreg_ldo17);
	else
		rc = vreg_disable(vreg_ldo17);

	if (rc) {
		pr_err("%s: LDO17 vreg enable failed (%d)\n",
			__func__, rc);
		return rc;
	}

	mdelay(5);		/* ensure power is stable */

#ifdef HDMI_RESET
	if (on) {
		gpio_set_value(37, 1);	/* bring reset line high */
		mdelay(10);		/* 10 msec before IO can be accessed */
	}
#endif

	return rc;
}

static struct lcdc_platform_data dtv_pdata = {
	.lcdc_power_save   = dtv_panel_power,
};

/* removed several lines */

static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0xFD,
};

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	if (!strcmp(name, "mddi_orise"))
		return -EPERM;

	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
	.mddi_prescan = 1,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};
#if defined(CONFIG_MSM_HW3D)
static struct resource resources_hw3d[] = {
	{
		.start	= 0xA0000000,
		.end	= 0xA00fffff,
		.flags	= IORESOURCE_MEM,
		.name	= "regs",
	},
	{
		.flags	= IORESOURCE_MEM,
		.name	= "smi",
	},
	{
		.flags	= IORESOURCE_MEM,
		.name	= "ebi",
	},
	{
		.start	= INT_GRAPHICS,
		.end	= INT_GRAPHICS,
		.flags	= IORESOURCE_IRQ,
		.name	= "gfx",
	},
};


static struct platform_device hw3d_device = {
	.name		= "msm_hw3d",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(resources_hw3d),
	.resource	= resources_hw3d,
};
#endif
static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
       .name = PMEM_KERNEL_EBI1_DATA_NAME,
	/* if no allocator_type, defaults to PMEM_ALLOCATORTYPE_BITMAP,
	* the only valid choice at this time. The board structure is
	* set to all zeros by the C runtime initialization and that is now
	* the enum value of PMEM_ALLOCATORTYPE_BITMAP, now forced to 0 in
	* include/linux/android_pmem.h.
	*/
       .cached = 0,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
       .name = "pmem_adsp",
       .allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
       .cached = 0,
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
       .name = "pmem_audio",
       .allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
       .cached = 0,
};

static struct platform_device android_pmem_kernel_ebi1_device = {
       .name = "android_pmem",
       .id = 1,
       .dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};

static struct platform_device android_pmem_adsp_device = {
       .name = "android_pmem",
       .id = 2,
       .dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct platform_device android_pmem_audio_device = {
       .name = "android_pmem",
       .id = 4,
       .dev = { .platform_data = &android_pmem_audio_pdata },
};

static struct kgsl_platform_data kgsl_pdata = {
#ifdef CONFIG_MSM_NPA_SYSTEM_BUS
	/* NPA Flow IDs */
	.high_axi_3d = MSM_AXI_FLOW_3D_GPU_HIGH,
	.high_axi_2d = MSM_AXI_FLOW_2D_GPU_HIGH,
#else
	/* AXI rates in KHz */
	.high_axi_3d = 192000,
	.high_axi_2d = 192000,
#endif
	.max_grp2d_freq = 0,
	.min_grp2d_freq = 0,
	.set_grp2d_async = NULL, /* HW workaround, run Z180 SYNC @ 192 MHZ */
	.max_grp3d_freq = 245760000,
	.min_grp3d_freq = 192000000,
	.set_grp3d_async = set_grp3d_async,
	.imem_clk_name = "imem_clk",
	.grp3d_clk_name = "grp_clk",
	.grp2d0_clk_name = "grp_2d_clk",
};

static struct resource msm_kgsl_resources[] = {
	{
		.name  = "kgsl_reg_memory",
		.start = MSM_GPU_REG_PHYS, /* 3D GRP address */
		.end   = MSM_GPU_REG_PHYS + MSM_GPU_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.name   = "kgsl_phys_memory",
		.flags  = IORESOURCE_MEM,
	},
	{
		.name  = "kgsl_yamato_irq",
		.start = INT_GRP_3D,
		.end   = INT_GRP_3D,
		.flags = IORESOURCE_IRQ,
	},
	{
		.name  = "kgsl_g12_reg_memory",
		.start = MSM_GPU_2D_REG_PHYS, /* Z180 base address */
		.end   = MSM_GPU_2D_REG_PHYS + MSM_GPU_2D_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "kgsl_g12_irq",
		.start = INT_GRP_2D,
		.end   = INT_GRP_2D,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device msm_device_kgsl = {
	.name = "kgsl",
	.id = -1,
	.num_resources = ARRAY_SIZE(msm_kgsl_resources),
	.resource = msm_kgsl_resources,
	.dev = {
		.platform_data = &kgsl_pdata,
	},
};


static int display_common_power(int on)
{
	int rc = 0;
    if (on) 
    {
        /*pmapp_display_clock_config(1);*/
    } else 
    {
        /*pmapp_display_clock_config(0);*/
    }
	return rc;
}
static int msm_fb_mddi_sel_clk(u32 *clk_rate)
{
	*clk_rate *= 2;
	return 0;
}

static struct mddi_platform_data mddi_pdata = {
	.mddi_power_save = display_common_power,
	.mddi_sel_clk = msm_fb_mddi_sel_clk,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 30,
	.mdp_core_clk_rate = 122880000,
};

/* removed several lines */
static int atv_dac_power(int on)
{
	int rc = 0;
	struct vreg *vreg_s4, *vreg_ldo9;

	vreg_s4 = vreg_get(NULL, "s4");
	if (IS_ERR(vreg_s4)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			   __func__, PTR_ERR(vreg_s4));
		return -1;
	}

	if (on) {
		rc = vreg_enable(vreg_s4);
		if (rc) {
			printk(KERN_ERR "%s: vreg_enable() = %d \n",
				__func__, rc);
			return rc;
		}
	} else {
		rc = vreg_disable(vreg_s4);
		if (rc) {
			pr_err("%s: S4 vreg enable failed (%d)\n",
				   __func__, rc);
			return rc;
		}
	}

	vreg_ldo9 = vreg_get(NULL, "gp1");
	if (IS_ERR(vreg_ldo9)) {
		rc = PTR_ERR(vreg_ldo9);
		pr_err("%s: gp1 vreg get failed (%d)\n",
			   __func__, rc);
		return rc;
	}


	if (on) {
		rc = vreg_enable(vreg_ldo9);
		if (rc) {
			pr_err("%s: LDO9 vreg enable failed (%d)\n",
				__func__, rc);
			return rc;
		}
	} else {
		rc = vreg_disable(vreg_ldo9);
		if (rc) {
			pr_err("%s: LDO20 vreg enable failed (%d)\n",
				   __func__, rc);
			return rc;
		}
	}

	return rc;
}

static struct tvenc_platform_data atv_pdata = {
	.pm_vid_en   = atv_dac_power,
};
static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("pmdh", &mddi_pdata);
	/* removed several lines */
	msm_fb_register_device("dtv", &dtv_pdata);
	msm_fb_register_device("tvenc", &atv_pdata);
}

/* removed several lines */

#if defined(CONFIG_MARIMBA_CORE) && \
   (defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
	.id     = -1
};

enum {
	BT_RFR,
	BT_CTS,
	BT_RX,
	BT_TX,
};

static struct msm_gpio bt_config_power_on[] = {
	{ GPIO_CFG(134, 1, GPIO_OUTPUT, GPIO_NO_PULL,   GPIO_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(135, 1, GPIO_INPUT,  GPIO_NO_PULL,   GPIO_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(136, 1, GPIO_INPUT,  GPIO_NO_PULL,   GPIO_2MA),
		"UART1DM_Rx" },
	{ GPIO_CFG(137, 1, GPIO_OUTPUT, GPIO_NO_PULL,   GPIO_2MA),
		"UART1DM_Tx" }
};

static struct msm_gpio bt_config_power_off[] = {
	{ GPIO_CFG(134, 0, GPIO_INPUT,  GPIO_PULL_DOWN,   GPIO_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(135, 0, GPIO_INPUT,  GPIO_PULL_DOWN,   GPIO_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(136, 0, GPIO_INPUT,  GPIO_PULL_DOWN,   GPIO_2MA),
		"UART1DM_Rx" },
	{ GPIO_CFG(137, 0, GPIO_INPUT,  GPIO_PULL_DOWN,   GPIO_2MA),
		"UART1DM_Tx" }
};

static struct msm_gpio bt_config_clock[] = {
	{ GPIO_CFG(34, 0, GPIO_OUTPUT,  GPIO_NO_PULL,    GPIO_2MA),
		"BT_REF_CLOCK_ENABLE" },
};

static int marimba_bt(int on)
{
	int rc;
	int i;
	struct marimba config = { .mod_id = MARIMBA_SLAVE_ID_MARIMBA };

	struct marimba_config_register {
		u8 reg;
		u8 value;
		u8 mask;
	};

	struct marimba_variant_register {
		const size_t size;
		const struct marimba_config_register *set;
	};

	const struct marimba_config_register *p;

	u8 version;

	const struct marimba_config_register v10_bt_on[] = {
		{ 0xE5, 0x0B, 0x0F },
		{ 0x05, 0x02, 0x07 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x21, 0x21 },
		{ 0xE3, 0x38, 0xFF },
		{ 0xE4, 0x06, 0xFF },
	};

	const struct marimba_config_register v10_bt_off[] = {
		{ 0xE5, 0x0B, 0x0F },
		{ 0x05, 0x08, 0x0F },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x00, 0x21 },
		{ 0xE3, 0x00, 0xFF },
		{ 0xE4, 0x00, 0xFF },
	};

	const struct marimba_config_register v201_bt_on[] = {
		{ 0x05, 0x08, 0x07 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x21, 0x21 },
		{ 0xE3, 0x38, 0xFF },
		{ 0xE4, 0x06, 0xFF },
	};

	const struct marimba_config_register v201_bt_off[] = {
		{ 0x05, 0x08, 0x07 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x00, 0x21 },
		{ 0xE3, 0x00, 0xFF },
		{ 0xE4, 0x00, 0xFF },
	};

	const struct marimba_config_register v210_bt_on[] = {
		{ 0xE9, 0x01, 0x01 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x21, 0x21 },
		{ 0xE3, 0x38, 0xFF },
		{ 0xE4, 0x06, 0xFF },
	};

	const struct marimba_config_register v210_bt_off[] = {
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x00, 0x21 },
		{ 0xE9, 0x00, 0x01 },
		{ 0xE3, 0x00, 0xFF },
		{ 0xE4, 0x00, 0xFF },
	};

	const struct marimba_variant_register bt_marimba[2][4] = {
		{
			{ ARRAY_SIZE(v10_bt_off), v10_bt_off },
			{ 0, NULL },
			{ ARRAY_SIZE(v201_bt_off), v201_bt_off },
			{ ARRAY_SIZE(v210_bt_off), v210_bt_off }
		},
		{
			{ ARRAY_SIZE(v10_bt_on), v10_bt_on },
			{ 0, NULL },
			{ ARRAY_SIZE(v201_bt_on), v201_bt_on },
			{ ARRAY_SIZE(v210_bt_on), v210_bt_on }
		}
	};

	on = on ? 1 : 0;

	rc = marimba_read_bit_mask(&config, 0x11,  &version, 1, 0x1F);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: version read failed: %d\n",
			__func__, rc);
		return rc;
	}

	if ((version >= ARRAY_SIZE(bt_marimba[on])) ||
	    (bt_marimba[on][version].size == 0)) {
		printk(KERN_ERR
			"%s: unsupported version\n",
			__func__);
		return -EIO;
	}

	p = bt_marimba[on][version].set;

	printk(KERN_INFO "%s: found version %d\n", __func__, version);

	for (i = 0; i < bt_marimba[on][version].size; i++) {
		u8 value = (p+i)->value;
		rc = marimba_write_bit_mask(&config,
			(p+i)->reg,
			&value,
			sizeof((p+i)->value),
			(p+i)->mask);
		if (rc < 0) {
			printk(KERN_ERR
				"%s: reg %d write failed: %d\n",
				__func__, (p+i)->reg, rc);
			return rc;
		}
		printk(KERN_INFO "%s: reg 0x%02x value 0x%02x mask 0x%02x\n",
				__func__, (p+i)->reg,
				value, (p+i)->mask);
	}
	return 0;
}

static const char *vregs_bt_name[] = {
	"gp16",
	"s2",
	"wlan"
};
static struct vreg *vregs_bt[ARRAY_SIZE(vregs_bt_name)];

static int bluetooth_power_regulators(int on)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(vregs_bt_name); i++) {
		rc = on ? vreg_enable(vregs_bt[i]) :
			  vreg_disable(vregs_bt[i]);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg %s %s failed (%d)\n",
				__func__, vregs_bt_name[i],
			       on ? "enable" : "disable", rc);
			return -EIO;
		}
	}
	return 0;
}

static int bluetooth_power(int on)
{
	int rc;
	const char *id = "BTPW";

	if (on) {
		rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 1300);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg level on failed (%d)\n",
				__func__, rc);
			return rc;
		}

		rc = bluetooth_power_regulators(on);
		if (rc < 0)
			return -EIO;

		rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_ON);
		if (rc < 0)
			return -EIO;

		if (machine_is_msm8x55_svlte_surf() ||
				machine_is_msm8x55_svlte_ffa())
			gpio_set_value(GPIO_PIN(bt_config_clock->gpio_cfg), 1);

		rc = marimba_bt(on);
		if (rc < 0)
			return -EIO;

  /*switch to CTRL latet to avoid QTR bt chip's bug*/
  /*
		rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_PIN_CTRL);
		if (rc < 0)
			return -EIO;
  */

		if (machine_is_msm8x55_svlte_surf() ||
				machine_is_msm8x55_svlte_ffa())
			gpio_set_value(GPIO_PIN(bt_config_clock->gpio_cfg), 0);

		rc = msm_gpios_enable(bt_config_power_on,
			ARRAY_SIZE(bt_config_power_on));

		if (rc < 0)
			return rc;

	} else {
		rc = msm_gpios_enable(bt_config_power_off,
					ARRAY_SIZE(bt_config_power_off));
		if (rc < 0)
			return rc;

		/* check for initial RFKILL block (power off) */
		if (platform_get_drvdata(&msm_bt_power_device) == NULL)
			goto out;

		rc = marimba_bt(on);
		if (rc < 0)
			return -EIO;

		rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_OFF);
		if (rc < 0)
			return -EIO;

		rc = bluetooth_power_regulators(on);
		if (rc < 0)
			return -EIO;

		rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 0);
		if (rc < 0) {
			printk(KERN_INFO "%s: vreg level off failed (%d)\n",
				__func__, rc);
		}
	}

out:
	printk(KERN_DEBUG "Bluetooth power switch: %d\n", on);

	return 0;
}

static void __init bt_power_init(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(vregs_bt_name); i++) {
		vregs_bt[i] = vreg_get(NULL, vregs_bt_name[i]);
		if (IS_ERR(vregs_bt[i])) {
			printk(KERN_ERR "%s: vreg get %s failed (%ld)\n",
			       __func__, vregs_bt_name[i],
			       PTR_ERR(vregs_bt[i]));
			return;
		}
	}

	if (machine_is_msm8x55_svlte_surf() || machine_is_msm8x55_svlte_ffa()) {
		rc = msm_gpios_request_enable(bt_config_clock,
					ARRAY_SIZE(bt_config_clock));
		if (rc < 0) {
			printk(KERN_ERR
				"%s: msm_gpios_request_enable failed (%d)\n",
					__func__, rc);
			return;
		}

		rc = gpio_direction_output(GPIO_PIN(bt_config_clock->gpio_cfg),
						0);
		if (rc < 0) {
			printk(KERN_ERR
				"%s: gpio_direction_output failed (%d)\n",
					__func__, rc);
			return;
		}
	}


	msm_bt_power_device.dev.platform_data = &bluetooth_power;
}
#else
#define bt_power_init(x) do {} while (0)
#endif


/*when msm7x30 start the i2c pull up power is not config *
*set the gp13 right pull up*/

#ifdef CONFIG_HUAWEI_KERNEL
	static void __init i2c_power_init(void)
	{
		struct vreg *vreg_gp13=NULL;
		int rc;
	
		vreg_gp13 = vreg_get(NULL, VREG_GP13_NAME);
		rc = vreg_set_level(vreg_gp13, VREG_GP13_VOLTAGE_VALUE);
		if (rc) {
		printk("%s: vreg_gp13  vreg_set_level failed \n", __func__);
	}
		rc = vreg_enable(vreg_gp13);
		if (rc) {
		pr_err("%s: vreg_gp13    vreg_enable failed \n", __func__);
	}
		mdelay(5);
	
	}
#endif
static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design 	= 2800,
	.voltage_max_design	= 4300,
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
};

static struct platform_device msm_batt_device = {
	.name 		    = "msm-battery",
	.id		    = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};

static struct platform_device *early_devices[] __initdata = {
#ifdef CONFIG_GPIOLIB
	&msm_gpio_devices[0],
	&msm_gpio_devices[1],
	&msm_gpio_devices[2],
	&msm_gpio_devices[3],
	&msm_gpio_devices[4],
	&msm_gpio_devices[5],
	&msm_gpio_devices[6],
	&msm_gpio_devices[7],
#endif
};

static char *msm_adc_device_names[] = { "LTC_ADC1", "LTC_ADC2", "LTC_ADC3" };

static struct msm_adc_platform_data msm_adc_pdata = {
       .dev_names = msm_adc_device_names,
       .num_adc = ARRAY_SIZE(msm_adc_device_names),
};

static struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};

static struct platform_device *devices[] __initdata = {
#if defined(CONFIG_SERIAL_MSM) || defined(CONFIG_MSM_SERIAL_DEBUGGER)
	&msm_device_uart2,
#endif
	&msm_device_smd,
	&msm_device_dmov,
	/* removed several lines */
#ifdef CONFIG_USB_FUNCTION
	&msm_device_hsusb_peripheral,
	&mass_storage_device,
#endif
#ifdef CONFIG_USB_MSM_OTG_72K
	&msm_device_otg,
#ifdef CONFIG_USB_GADGET
	&msm_device_gadget_peripheral,
#endif
#endif
#ifdef CONFIG_USB_ANDROID
	&mass_storage_device,
	&android_usb_device,
#endif
	&qsd_device_spi,
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi6,
	&msm_device_ssbi7,
#endif
	&android_pmem_device,
	&msm_fb_device,
	/* removed several lines */
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
	/* removed several lines */
	&android_pmem_kernel_ebi1_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&msm_device_i2c,
	&msm_device_i2c_2,
	&msm_device_uart_dm1,
	&hs_device,
#ifdef CONFIG_MSM7KV2_AUDIO
	&msm_aictl_device,
	&msm_mi2s_device,
	&msm_lpa_device,
	&msm_aux_pcm_device,
#endif
	&msm_device_adspdec,
	&qup_device_i2c,
#if defined(CONFIG_MARIMBA_CORE) && \
   (defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
	&msm_bt_power_device,
#endif
	&msm_device_kgsl,
#ifdef CONFIG_MT9T013
	&msm_camera_sensor_mt9t013,
#endif
#ifdef CONFIG_MT9D112
	&msm_camera_sensor_mt9d112,
#endif
#ifdef CONFIG_HUAWEI_SENSOR_OV7690
	&msm_camera_sensor_ov7690,
#endif


#ifdef CONFIG_HUAWEI_SENSOR_HIMAX0356	
    &msm_camera_sensor_himax0356,
#endif 

#ifdef CONFIG_HUAWEI_SENSOR_S5K4E1GX
	&msm_camera_sensor_s5k4e1gx,
#endif
#ifdef CONFIG_HUAWEI_SENSOR_OV5647_SUNNY
	&msm_camera_sensor_ov5647_sunny,
#endif
#ifdef CONFIG_S5K3E2FX
	&msm_camera_sensor_s5k3e2fx,
#endif
#ifdef CONFIG_MT9P012
	&msm_camera_sensor_mt9p012,
#endif
#ifdef CONFIG_VX6953
	&msm_camera_sensor_vx6953,
#endif
#ifdef CONFIG_SN12M0PZ
	&msm_camera_sensor_sn12m0pz,
#endif
	&msm_device_vidc_720p,
#ifdef CONFIG_MSM_GEMINI
	&msm_gemini_device,
#endif
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
	&msm_device_tsif,
#endif
	&msm_batt_device,
#ifdef CONFIG_HUAWEI_FEATURE_RGB_KEY_LIGHT
	&rgb_leds_device,
#endif
	&msm_adc_device,
	
#ifdef CONFIG_HUAWEI_LEDS_PMIC
    &msm_device_pmic_leds,
#endif
	
};

static struct msm_gpio msm_i2c_gpios_hw[] = {
	{ GPIO_CFG(70, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_16MA), "i2c_scl" },
	{ GPIO_CFG(71, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_16MA), "i2c_sda" },
};

static struct msm_gpio msm_i2c_gpios_io[] = {
	{ GPIO_CFG(70, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), "i2c_scl" },
	{ GPIO_CFG(71, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), "i2c_sda" },
};

static struct msm_gpio qup_i2c_gpios_io[] = {
	{ GPIO_CFG(16, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), "qup_scl" },
	{ GPIO_CFG(17, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), "qup_sda" },
};
static struct msm_gpio qup_i2c_gpios_hw[] = {
	{ GPIO_CFG(16, 2, GPIO_INPUT, GPIO_NO_PULL, GPIO_16MA), "qup_scl" },
	{ GPIO_CFG(17, 2, GPIO_INPUT, GPIO_NO_PULL, GPIO_16MA), "qup_sda" },
};

static void
msm_i2c_gpio_config(int adap_id, int config_type)
{
	struct msm_gpio *msm_i2c_table;

	/* Each adapter gets 2 lines from the table */
	if (adap_id > 0)
		return;
	if (config_type)
		msm_i2c_table = &msm_i2c_gpios_hw[adap_id*2];
	else
		msm_i2c_table = &msm_i2c_gpios_io[adap_id*2];
	msm_gpios_enable(msm_i2c_table, 2);
}

static struct vreg *qup_vreg;
static void
qup_i2c_gpio_config(int adap_id, int config_type)
{
	int rc = 0;
	struct msm_gpio *qup_i2c_table;
	/* Each adapter gets 2 lines from the table */
	if (adap_id != 4)
		return;
	if (config_type)
		qup_i2c_table = qup_i2c_gpios_hw;
	else
		qup_i2c_table = qup_i2c_gpios_io;
	rc = msm_gpios_enable(qup_i2c_table, 2);
	if (rc < 0)
		printk(KERN_ERR "QUP GPIO enable failed: %d\n", rc);
	if (qup_vreg) {
		int rc = vreg_set_level(qup_vreg, 1800);
		if (rc) {
			pr_err("%s: vreg LVS1 set level failed (%d)\n",
			__func__, rc);
		}
		rc = vreg_enable(qup_vreg);
		if (rc) {
			pr_err("%s: vreg_enable() = %d \n",
			__func__, rc);
		}
	}
}

static struct msm_i2c_platform_data msm_i2c_pdata = {
	.clk_freq = 100000,
	.pri_clk = 70,
	.pri_dat = 71,
	.rmutex  = 1,
	.rsl_id = "D:I2C02000021",
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_init(void)
{
	if (msm_gpios_request(msm_i2c_gpios_hw, ARRAY_SIZE(msm_i2c_gpios_hw)))
		pr_err("failed to request I2C gpios\n");

	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
	
	#ifdef CONFIG_HUAWEI_KERNEL
	i2c_power_init();
	#endif
}

static struct msm_i2c_platform_data msm_i2c_2_pdata = {
	.clk_freq = 100000,
	.rmutex  = 0,
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_2_init(void)
{
	msm_device_i2c_2.dev.platform_data = &msm_i2c_2_pdata;
}

static struct msm_i2c_platform_data qup_i2c_pdata = {
	.clk_freq = 384000,
	.pclk = "camif_pad_pclk",
	.msm_i2c_config_gpio = qup_i2c_gpio_config,
};

static void __init qup_device_i2c_init(void)
{
	if (msm_gpios_request(qup_i2c_gpios_hw, ARRAY_SIZE(qup_i2c_gpios_hw)))
		pr_err("failed to request I2C gpios\n");

	qup_device_i2c.dev.platform_data = &qup_i2c_pdata;
	qup_vreg = vreg_get(NULL, "lvsw1");
	if (IS_ERR(qup_vreg)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(qup_vreg));
	}
}

#ifdef CONFIG_I2C_SSBI
static struct msm_ssbi_platform_data msm_i2c_ssbi6_pdata = {
	.rsl_id = "D:PMIC_SSBI",
	.controller_type = MSM_SBI_CTRL_SSBI2,
};

static struct msm_ssbi_platform_data msm_i2c_ssbi7_pdata = {
	.rsl_id = "D:CODEC_SSBI",
	.controller_type = MSM_SBI_CTRL_SSBI,
};
#endif

static struct msm_acpu_clock_platform_data msm7x30_clock_data = {
	.acpu_switch_time_us = 50,
	.vdd_switch_time_us = 62,
};

static void __init msm7x30_init_irq(void)
{
	msm_init_irq();
}

/* removed several lines */

struct vreg *vreg_s3;
struct vreg *vreg_mmc;

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT))

struct sdcc_gpio {
	struct msm_gpio *cfg_data;
	uint32_t size;
};
#if defined(CONFIG_MMC_MSM_SDC1_SUPPORT)
static struct msm_gpio sdc1_lvlshft_cfg_data[] = {
	{GPIO_CFG(35, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_16MA), "sdc1_lvlshft"},
};
#endif
static struct msm_gpio sdc1_cfg_data[] = {
	{GPIO_CFG(38, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), "sdc1_clk"},
	{GPIO_CFG(39, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc1_cmd"},
	{GPIO_CFG(40, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc1_dat_3"},
	{GPIO_CFG(41, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc1_dat_2"},
	{GPIO_CFG(42, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc1_dat_1"},
	{GPIO_CFG(43, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc1_dat_0"},
};

static struct msm_gpio sdc2_cfg_data[] = {
	{GPIO_CFG(64, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), "sdc2_clk"},
	{GPIO_CFG(65, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_cmd"},
	{GPIO_CFG(66, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_3"},
	{GPIO_CFG(67, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_2"},
	{GPIO_CFG(68, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_1"},
	{GPIO_CFG(69, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_0"},

#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	{GPIO_CFG(115, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_4"},
	{GPIO_CFG(114, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_5"},
	{GPIO_CFG(113, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_6"},
	{GPIO_CFG(112, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_7"},
#endif
};

static struct msm_gpio sdc3_cfg_data[] = {
	{GPIO_CFG(110, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), "sdc3_clk"},
	{GPIO_CFG(111, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_cmd"},
	{GPIO_CFG(116, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_3"},
	{GPIO_CFG(117, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_2"},
	{GPIO_CFG(118, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_1"},
	{GPIO_CFG(119, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_0"},
};

static struct msm_gpio sdc4_cfg_data[] = {
	{GPIO_CFG(58, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), "sdc4_clk"},
	{GPIO_CFG(59, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_cmd"},
	{GPIO_CFG(60, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_3"},
	{GPIO_CFG(61, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_2"},
	{GPIO_CFG(62, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_1"},
	{GPIO_CFG(63, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_0"},
};

static struct sdcc_gpio sdcc_cfg_data[] = {
	{
		.cfg_data = sdc1_cfg_data,
		.size = ARRAY_SIZE(sdc1_cfg_data),
	},
	{
		.cfg_data = sdc2_cfg_data,
		.size = ARRAY_SIZE(sdc2_cfg_data),
	},
	{
		.cfg_data = sdc3_cfg_data,
		.size = ARRAY_SIZE(sdc3_cfg_data),
	},
	{
		.cfg_data = sdc4_cfg_data,
		.size = ARRAY_SIZE(sdc4_cfg_data),
	},
};

struct sdcc_vreg {
	struct vreg *vreg_data;
	unsigned level;
};

static struct sdcc_vreg sdcc_vreg_data[4];

static unsigned long vreg_sts, gpio_sts;

static uint32_t msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_gpio *curr;

	curr = &sdcc_cfg_data[dev_id - 1];

	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return rc;

	if (enable) {
		set_bit(dev_id, &gpio_sts);
		rc = msm_gpios_request_enable(curr->cfg_data, curr->size);
		if (rc)
			printk(KERN_ERR "%s: Failed to turn on GPIOs for slot %d\n",
				__func__,  dev_id);
	} else {
		clear_bit(dev_id, &gpio_sts);
		msm_gpios_disable_free(curr->cfg_data, curr->size);
	}

	return rc;
}

static uint32_t msm_sdcc_setup_vreg(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_vreg *curr;
	static int enabled_once[] = {0, 0, 0, 0};

	curr = &sdcc_vreg_data[dev_id - 1];

	if (!(test_bit(dev_id, &vreg_sts)^enable))
		return rc;

	if (!enable || enabled_once[dev_id - 1])
		return 0;

	if (enable) {
		set_bit(dev_id, &vreg_sts);
		rc = vreg_set_level(curr->vreg_data, curr->level);
		if (rc) {
			printk(KERN_ERR "%s: vreg_set_level() = %d \n",
					__func__, rc);
		}
		rc = vreg_enable(curr->vreg_data);
		if (rc) {
			printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		}
		enabled_once[dev_id - 1] = 1;
	} else {
		clear_bit(dev_id, &vreg_sts);
		rc = vreg_disable(curr->vreg_data);
		if (rc) {
			printk(KERN_ERR "%s: vreg_disable() = %d \n",
					__func__, rc);
		}
	}
	return rc;
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	rc = msm_sdcc_setup_gpio(pdev->id, (vdd ? 1 : 0));
	if (rc)
		goto out;

	if (pdev->id == 4) /* S3 is always ON and cannot be disabled */
		rc = msm_sdcc_setup_vreg(pdev->id, (vdd ? 1 : 0));
out:
	return rc;
}
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
static unsigned int msm7x30_sdcc_slot_status(struct device *dev)
{
	return (unsigned int)
		gpio_get_value(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SD_DET - 1));
}
#endif

#ifndef CONFIG_HUAWEI_KERNEL
static int msm_sdcc_get_wpswitch(struct device *dv)
{
	void __iomem *wp_addr = 0;
	uint32_t ret = 0;
	struct platform_device *pdev;

	if (!(machine_is_msm7x30_surf()))
		return -1;
	pdev = container_of(dv, struct platform_device, dev);

	wp_addr = ioremap(FPGA_SDCC_STATUS, 4);
	if (!wp_addr) {
		pr_err("%s: Could not remap %x\n", __func__, FPGA_SDCC_STATUS);
		return -ENOMEM;
	}

	ret = (((readl(wp_addr) >> 4) >> (pdev->id-1)) & 0x01);
	pr_info("%s: WP Status for Slot %d = 0x%x \n", __func__,
							pdev->id, ret);
	iounmap(wp_addr);

	return ret;
}
#endif
#endif /*CONFIG_HUAWEI_KERNEL*/

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static struct mmc_platform_data msm7x30_sdc1_data = {
	.ocr_mask	= MMC_VDD_165_195,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDC1_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct mmc_platform_data msm7x30_sdc2_data = {
	.ocr_mask	= MMC_VDD_165_195 | MMC_VDD_27_28,
	.translate_vdd	= msm_sdcc_setup_power,
#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
#ifdef CONFIG_MMC_MSM_SDC2_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 1,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct mmc_platform_data msm7x30_sdc3_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
	.sdiowakeup_irq = MSM_GPIO_TO_INT(118),
#endif
#ifdef CONFIG_MMC_MSM_SDC3_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 1,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct mmc_platform_data msm7x30_sdc4_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status      = msm7x30_sdcc_slot_status,
	.status_irq  = PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, PMIC_GPIO_SD_DET - 1),
	.irq_flags   = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
#ifndef CONFIG_HUAWEI_KERNEL
	.wpswitch    = msm_sdcc_get_wpswitch,
#endif
#ifdef CONFIG_MMC_MSM_SDC4_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
#ifdef CONFIG_HUAWEI_KERNEL
	.msmsdcc_fmax	= 40960000,
#else
	.msmsdcc_fmax	= 49152000,
#endif
	.nonremovable	= 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static void msm_sdc1_lvlshft_enable(void)
{
	int rc;

	/* Enable LDO5, an input to the FET that powers slot 1 */
	rc = vreg_set_level(vreg_mmc, 2850);
	if (rc)
		printk(KERN_ERR "%s: vreg_set_level() = %d \n",	__func__, rc);

	rc = vreg_enable(vreg_mmc);
	if (rc)
		printk(KERN_ERR "%s: vreg_enable() = %d \n", __func__, rc);

	/* Enable GPIO 35, to turn on the FET that powers slot 1 */
	rc = msm_gpios_request_enable(sdc1_lvlshft_cfg_data,
				ARRAY_SIZE(sdc1_lvlshft_cfg_data));
	if (rc)
		printk(KERN_ERR "%s: Failed to enable GPIO 35\n", __func__);

	rc = gpio_direction_output(GPIO_PIN(sdc1_lvlshft_cfg_data[0].gpio_cfg),
				1);
	if (rc)
		printk(KERN_ERR "%s: Failed to turn on GPIO 35\n", __func__);
}
#endif

static void __init msm7x30_init_mmc(void)
{
	vreg_s3 = vreg_get(NULL, "s3");
	if (IS_ERR(vreg_s3)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_s3));
		return;
	}

	vreg_mmc = vreg_get(NULL, "mmc");
	if (IS_ERR(vreg_mmc)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_mmc));
		return;
	}

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	if (machine_is_msm7x30_fluid() || (machine_is_msm7x30_u8800()) || (machine_is_msm7x30_u8820())) {
		msm7x30_sdc1_data.ocr_mask =  MMC_VDD_27_28 | MMC_VDD_28_29;
		msm_sdc1_lvlshft_enable();
	}
	sdcc_vreg_data[0].vreg_data = vreg_s3;
	sdcc_vreg_data[0].level = 1800;
	msm_add_sdcc(1, &msm7x30_sdc1_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	if (machine_is_msm8x55_svlte_surf())
		msm7x30_sdc2_data.msmsdcc_fmax =  24576000;
	sdcc_vreg_data[1].vreg_data = vreg_s3;
	sdcc_vreg_data[1].level = 1800;
	msm_add_sdcc(2, &msm7x30_sdc2_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	sdcc_vreg_data[2].vreg_data = vreg_s3;
	sdcc_vreg_data[2].level = 1800;
	msm_add_sdcc(3, &msm7x30_sdc3_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	sdcc_vreg_data[3].vreg_data = vreg_mmc;
	sdcc_vreg_data[3].level = 2850;
	msm_add_sdcc(4, &msm7x30_sdc4_data);
#endif

}

/* removed several lines */

#ifdef CONFIG_SERIAL_MSM_CONSOLE
static struct msm_gpio uart2_config_data[] = {
	{ GPIO_CFG(49, 2, GPIO_OUTPUT,  GPIO_PULL_DOWN, GPIO_2MA), "UART2_RFR"},
	{ GPIO_CFG(50, 2, GPIO_INPUT,   GPIO_PULL_DOWN, GPIO_2MA), "UART2_CTS"},
	{ GPIO_CFG(51, 2, GPIO_INPUT,   GPIO_PULL_DOWN, GPIO_2MA), "UART2_Rx"},
	{ GPIO_CFG(52, 2, GPIO_OUTPUT,  GPIO_PULL_DOWN, GPIO_2MA), "UART2_Tx"},
};

static void msm7x30_init_uart2(void)
{
	msm_gpios_request_enable(uart2_config_data,
			ARRAY_SIZE(uart2_config_data));

}
#endif

/* TSIF begin */
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)

#define TSIF_B_SYNC      GPIO_CFG(37, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)
#define TSIF_B_DATA      GPIO_CFG(36, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)
#define TSIF_B_EN        GPIO_CFG(35, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)
#define TSIF_B_CLK       GPIO_CFG(34, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)

static const struct msm_gpio tsif_gpios[] = {
	{ .gpio_cfg = TSIF_B_CLK,  .label =  "tsif_clk", },
	{ .gpio_cfg = TSIF_B_EN,   .label =  "tsif_en", },
	{ .gpio_cfg = TSIF_B_DATA, .label =  "tsif_data", },
	{ .gpio_cfg = TSIF_B_SYNC, .label =  "tsif_sync", },
};

static struct msm_tsif_platform_data tsif_platform_data = {
	.num_gpios = ARRAY_SIZE(tsif_gpios),
	.gpios = tsif_gpios,
	.tsif_pclk = "tsif_pclk",
	.tsif_ref_clk = "tsif_ref_clk",
};
#endif /* defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE) */
/* TSIF end   */

static void __init pmic8058_leds_init(void)
{
	if (machine_is_msm7x30_surf()) {
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].platform_data
			= &pm8058_surf_leds_data;
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].data_size
			= sizeof(pm8058_surf_leds_data);
	} else if (machine_is_msm7x30_ffa()) {
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].platform_data
			= &pm8058_ffa_leds_data;
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].data_size
			= sizeof(pm8058_ffa_leds_data);
	}
}

static struct msm_spm_platform_data msm_spm_data __initdata = {
	.reg_base_addr = MSM_SAW_BASE,

	.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x05,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x18,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x00006666,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFF000666,

	.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x03,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

	.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

	.awake_vlevel = 0xF2,
	.retention_vlevel = 0xE0,
	.collapse_vlevel = 0x72,
	.retention_mid_vlevel = 0xE0,
	.collapse_mid_vlevel = 0xE0,

	.vctl_timeout_us = 50,
};

#if defined(CONFIG_TOUCHSCREEN_TSC2007) || \
	defined(CONFIG_TOUCHSCREEN_TSC2007_MODULE)

#define TSC2007_TS_PEN_INT	20

static struct msm_gpio tsc2007_config_data[] = {
	{ GPIO_CFG(TSC2007_TS_PEN_INT, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),
	"tsc2007_irq" },
};

static struct vreg *vreg_tsc_s3;
static struct vreg *vreg_tsc_s2;

static int tsc2007_init(void)
{
	int rc;

	vreg_tsc_s3 = vreg_get(NULL, "s3");
	if (IS_ERR(vreg_tsc_s3)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_tsc_s3));
		return -ENODEV;
	}

	rc = vreg_set_level(vreg_tsc_s3, 1800);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_set_level;
	}

	rc = vreg_enable(vreg_tsc_s3);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_set_level;
	}

	vreg_tsc_s2 = vreg_get(NULL, "s2");
	if (IS_ERR(vreg_tsc_s2)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_tsc_s2));
		goto fail_vreg_get;
	}

	rc = vreg_set_level(vreg_tsc_s2, 1300);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_s2_level;
	}

	rc = vreg_enable(vreg_tsc_s2);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_s2_level;
	}

	rc = msm_gpios_request_enable(tsc2007_config_data,
			ARRAY_SIZE(tsc2007_config_data));
	if (rc) {
		pr_err("%s: Unable to request gpios\n", __func__);
		goto fail_gpio_req;
	}

	return 0;

fail_gpio_req:
	vreg_disable(vreg_tsc_s2);
fail_vreg_s2_level:
	vreg_put(vreg_tsc_s2);
fail_vreg_get:
	vreg_disable(vreg_tsc_s3);
fail_vreg_set_level:
	vreg_put(vreg_tsc_s3);
	return rc;
}

static int tsc2007_get_pendown_state(void)
{
	int rc;

	rc = gpio_get_value(TSC2007_TS_PEN_INT);
	if (rc < 0) {
		pr_err("%s: MSM GPIO %d read failed\n", __func__,
						TSC2007_TS_PEN_INT);
		return rc;
	}

	return (rc == 0 ? 1 : 0);
}

static void tsc2007_exit(void)
{
	vreg_disable(vreg_tsc_s3);

	msm_gpios_disable_free(tsc2007_config_data,
		ARRAY_SIZE(tsc2007_config_data));
}

static struct tsc2007_platform_data tsc2007_ts_data = {
	.model = 2007,
	.x_plate_ohms = 300,
	.irq_flags    = IRQF_TRIGGER_LOW,
	.init_platform_hw = tsc2007_init,
	.exit_platform_hw = tsc2007_exit,
	.invert_x	  = true,
	.invert_y	  = true,
	/* REVISIT: Temporary fix for reversed pressure */
	.invert_z1	  = true,
	.invert_z2	  = true,
	.get_pendown_state = tsc2007_get_pendown_state,
};

static struct i2c_board_info tsc_i2c_board_info[] = {
	{
		I2C_BOARD_INFO("tsc2007", 0x48),
		.irq		= MSM_GPIO_TO_INT(TSC2007_TS_PEN_INT),
		.platform_data = &tsc2007_ts_data,
	},
};
#endif

static const char *vregs_isa1200_name[] = {
	"gp7",
	"gp10",
};

static const int vregs_isa1200_val[] = {
	1800,
	2600,
};
static struct vreg *vregs_isa1200[ARRAY_SIZE(vregs_isa1200_name)];

static struct msm_gpio fluid_hap_shift_lvl_gpio[] = {
       { GPIO_CFG(HAP_LVL_SHFT_MSM_GPIO, 0, GPIO_OUTPUT, GPIO_NO_PULL,
		GPIO_2MA), "haptics_shft_lvl_oe"},
};

static int isa1200_power(int vreg_on)
{
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(vregs_isa1200_name); i++) {
		if (!vregs_isa1200[i]) {
			printk(KERN_ERR "%s: vreg_get %s failed (%d)\n",
				__func__, vregs_isa1200_name[i], rc);
			goto vreg_fail;
		}

		rc = vreg_on ? vreg_enable(vregs_isa1200[i]) :
			  vreg_disable(vregs_isa1200[i]);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg %s %s failed (%d)\n",
				__func__, vregs_isa1200_name[i],
			       vreg_on ? "enable" : "disable", rc);
			goto vreg_fail;
		}
	}
	return 0;

vreg_fail:
	while (i)
		vreg_disable(vregs_isa1200[--i]);
	return rc;
}

static int hap_lvl_shft_config(void)
{
	int rc;

	rc = msm_gpios_request_enable(fluid_hap_shift_lvl_gpio,
			ARRAY_SIZE(fluid_hap_shift_lvl_gpio));
	if (rc) {
		pr_err("%s gpio_request_enable failed rc=%d\n",
				__func__, rc);
		goto gpio_request_fail;
	}

	gpio_set_value(HAP_LVL_SHFT_MSM_GPIO, 1);

	return 0;
gpio_request_fail:
	return rc;
}

static struct isa1200_platform_data isa1200_1_pdata = {
	.name = "vibrator",
	.power_on = isa1200_power,
	.pwm_ch_id = 1, /*channel id*/
	/*gpio to enable haptic*/
	.hap_en_gpio = PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HAP_ENABLE),
	.max_timeout = 15000,
};

static struct i2c_board_info msm_isa1200_board_info[] = {
	{
		I2C_BOARD_INFO("isa1200_1", 0x90>>1),
		.platform_data = &isa1200_1_pdata,
	},
};

static void isa1200_init(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(vregs_isa1200_name); i++) {
		vregs_isa1200[i] = vreg_get(NULL, vregs_isa1200_name[i]);
		if (IS_ERR(vregs_isa1200[i])) {
			printk(KERN_ERR "%s: vreg get %s failed (%ld)\n",
				__func__, vregs_isa1200_name[i],
				PTR_ERR(vregs_isa1200[i]));
			rc = PTR_ERR(vregs_isa1200[i]);
			goto vreg_get_fail;
		}
		rc = vreg_set_level(vregs_isa1200[i],
				vregs_isa1200_val[i]);
		if (rc) {
			printk(KERN_ERR "%s: vreg_set_level() = %d\n",
				__func__, rc);
			goto vreg_get_fail;
		}
	}

	rc = hap_lvl_shft_config();
	if (rc) {
		pr_err("%s: Haptic level shifter gpio %d configuration"
			" failed\n", __func__, HAP_LVL_SHFT_MSM_GPIO);
		goto vreg_get_fail;
	}

	i2c_register_board_info(0, msm_isa1200_board_info,
		ARRAY_SIZE(msm_isa1200_board_info));
	return;
vreg_get_fail:
	while (i)
		vreg_put(vregs_isa1200[--i]);
}

static int kp_flip_mpp_config(void)
{
	return pm8058_mpp_config_digital_in(PM_FLIP_MPP,
		PM8058_MPP_DIG_LEVEL_S3, PM_MPP_DIN_TO_INT);
}

static struct flip_switch_pdata flip_switch_data = {
	.name = "kp_flip_switch",
	.flip_gpio = PM8058_GPIO_PM_TO_SYS(PM8058_GPIOS) + PM_FLIP_MPP,
	.left_key = KEY_OPEN,
	.right_key = KEY_CLOSE,
	.active_low = 0,
	.wakeup = 1,
	.flip_mpp_config = kp_flip_mpp_config,
};

static struct platform_device flip_switch_device = {
	.name   = "kp_flip_switch",
	.id	= -1,
	.dev    = {
		.platform_data = &flip_switch_data,
	}
};

static const char *vregs_tma300_name[] = {
	"gp6",
	"gp7",
};

static const int vregs_tma300_val[] = {
	3050,
	1800,
};
static struct vreg *vregs_tma300[ARRAY_SIZE(vregs_tma300_name)];

static int tma300_power(int vreg_on)
{
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(vregs_tma300_name); i++) {
		if (!vregs_tma300[i]) {
			printk(KERN_ERR "%s: vreg_get %s failed (%d)\n",
				__func__, vregs_tma300_name[i], rc);
			goto vreg_fail;
		}

		rc = vreg_on ? vreg_enable(vregs_tma300[i]) :
			  vreg_disable(vregs_tma300[i]);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg %s %s failed (%d)\n",
				__func__, vregs_tma300_name[i],
			       vreg_on ? "enable" : "disable", rc);
			goto vreg_fail;
		}
	}
	return 0;

vreg_fail:
	while (i)
		vreg_disable(vregs_tma300[--i]);
	return rc;
}

/*virtual key support */
static ssize_t tma300_vkeys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
	__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":80:904:160:210"
	":" __stringify(EV_KEY) ":" __stringify(KEY_MENU) ":240:904:160:210"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":400:904:160:210"
	"\n");
}

static struct kobj_attribute tma300_vkeys_attr = {
	.attr = {
		.name = "virtualkeys.msm_tma300_ts",
		.mode = S_IRUGO,
	},
	.show = &tma300_vkeys_show,
};

static struct attribute *tma300_properties_attrs[] = {
	&tma300_vkeys_attr.attr,
	NULL
};

static struct attribute_group tma300_properties_attr_group = {
	.attrs = tma300_properties_attrs,
};

static struct kobject *properties_kobj;

#define TS_GPIO_IRQ 150

static int tma300_dev_setup(bool enable)
{
	int i, rc;

	if (enable) {
		/* get voltage sources */
		for (i = 0; i < ARRAY_SIZE(vregs_tma300_name); i++) {
			vregs_tma300[i] = vreg_get(NULL, vregs_tma300_name[i]);
			if (IS_ERR(vregs_tma300[i])) {
				pr_err("%s: vreg get %s failed (%ld)\n",
					__func__, vregs_tma300_name[i],
					PTR_ERR(vregs_tma300[i]));
				rc = PTR_ERR(vregs_tma300[i]);
				goto vreg_get_fail;
			}
			rc = vreg_set_level(vregs_tma300[i],
					vregs_tma300_val[i]);
			if (rc) {
				pr_err("%s: vreg_set_level() = %d\n",
					__func__, rc);
				i++;
				goto vreg_get_fail;
			}
		}

		/* enable interrupt gpio */
		rc = gpio_tlmm_config(GPIO_CFG(TS_GPIO_IRQ, 0, GPIO_INPUT,
				GPIO_PULL_UP, GPIO_6MA), GPIO_ENABLE);
		if (rc) {
			pr_err("%s: Could not configure gpio %d\n",
					__func__, TS_GPIO_IRQ);
			goto vreg_get_fail;
		}

		rc = gpio_request(TS_GPIO_IRQ, "ts_irq");
		if (rc) {
			pr_err("%s: unable to request gpio %d (%d)\n",
					__func__, TS_GPIO_IRQ, rc);
			goto vreg_get_fail;
		}

		/* virtual keys */
		properties_kobj = kobject_create_and_add("board_properties",
					NULL);
		if (properties_kobj)
			rc = sysfs_create_group(properties_kobj,
				&tma300_properties_attr_group);
		if (!properties_kobj || rc)
			pr_err("%s: failed to create board_properties\n",
					__func__);
	} else {
		/* put voltage sources */
		for (i = 0; i < ARRAY_SIZE(vregs_tma300_name); i++)
			vreg_put(vregs_tma300[i]);
		/* free gpio */
		gpio_free(TS_GPIO_IRQ);
		/* destroy virtual keys */
		if (properties_kobj)
			sysfs_remove_group(properties_kobj,
				&tma300_properties_attr_group);
	}
	return 0;

vreg_get_fail:
	while (i)
		vreg_put(vregs_tma300[--i]);
	return rc;
}

#ifdef CONFIG_USB_AUTO_INSTALL
/* provide a method to map pid_index to usb_pid, 
 * pid_index is kept in NV(4526). 
 * At power up, pid_index is read in modem and transfer to app in share memory.
 * pid_index can be modified through write file fixusb(msm_hsusb_store_fixusb).
*/
u16 pid_index_to_pid(u32 pid_index)
{
    u16 usb_pid = 0xFFFF;
    
    switch(pid_index)
    {
        case CDROM_INDEX:
            usb_pid = curr_usb_pid_ptr->cdrom_pid;
            break;
        case NORM_INDEX:
            usb_pid = curr_usb_pid_ptr->norm_pid;
            break;
        case AUTH_INDEX:
            usb_pid = curr_usb_pid_ptr->auth_pid;
            break;
        /* add new pid config for google */
        case GOOGLE_INDEX:
            usb_pid = curr_usb_pid_ptr->google_pid;
            break;
        /* new requirement: usb tethering */
        case GOOGLE_WLAN_INDEX:
            usb_pid = curr_usb_pid_ptr->wlan_pid;
            break;
        /* set the USB pid to multiport when the index is 0
           This is happened when the NV is not set or set 
           to zero 
        */
        case ORI_INDEX:
        default:
            usb_pid = curr_usb_pid_ptr->norm_pid;
            break;
    }

    USB_PR("%s, pid_index=%d, usb_pid=0x%x\n", __func__, pid_index, usb_pid);
    
    return usb_pid;
}

/* add new pid config for google */
void set_usb_pid_sn(u32 pid_index)
{
    switch(pid_index)
    {
        /* new requirement: usb tethering */
        case GOOGLE_WLAN_INDEX:
            USB_PR("set pid=0x%x, sn=%s\n", GOOGLE_WLAN_INDEX, "");
            android_set_product_id(PID_WLAN);
            set_usb_sn((char *)NULL);
            break;
        case GOOGLE_INDEX:
            USB_PR("set pid=0x%x, sn=%s\n", PID_GOOGLE_MS, usb_para_data.usb_para.usb_serial);
            android_set_product_id(PID_GOOGLE_MS);
            set_usb_sn(usb_para_data.usb_para.usb_serial);
            break;
            
        case NORM_INDEX:
            USB_PR("set pid=0x%x, sn=%s\n", curr_usb_pid_ptr->norm_pid, USB_SN_STRING);
            android_set_product_id(curr_usb_pid_ptr->norm_pid);
            set_usb_sn(USB_SN_STRING);
            break;
            
        case CDROM_INDEX:
            USB_PR("set pid=0x%x, sn=%s\n", curr_usb_pid_ptr->cdrom_pid, "");
            android_set_product_id(curr_usb_pid_ptr->cdrom_pid);
            set_usb_sn((char *)NULL);
            break;
            
        case ORI_INDEX:
            USB_PR("set pid=0x%x, sn=%s\n", curr_usb_pid_ptr->norm_pid, "");
            android_set_product_id(curr_usb_pid_ptr->norm_pid);
            set_usb_sn((char *)NULL);
            break;
            
        case AUTH_INDEX:
            USB_PR("set pid=0x%x, sn=%s\n", curr_usb_pid_ptr->auth_pid, "");
            android_set_product_id(curr_usb_pid_ptr->auth_pid);
            set_usb_sn((char *)NULL);
            break;
            
        default:
            USB_PR("set pid=0x%x, sn=%s\n", curr_usb_pid_ptr->norm_pid, "");
            android_set_product_id(curr_usb_pid_ptr->norm_pid);
            set_usb_sn((char *)NULL);
            break;
    }

}

/*  
 * Get usb parameter from share memory and set usb serial number accordingly.
 */
static void proc_usb_para(void)
{
    smem_huawei_vender *usb_para_ptr = NULL;
    //u16 pid;
    char *vender_name="t-mobile";

    USB_PR("< %s\n", __func__);

    /* initialize */
    usb_para_info.usb_pid_index = 0;
    usb_para_info.usb_pid = PID_NORMAL;
    
    /* now the smem_id_vendor0 smem id is a new struct */
    usb_para_ptr = (smem_huawei_vender*)smem_alloc(SMEM_ID_VENDOR0, sizeof(smem_huawei_vender));
    if (!usb_para_ptr)
    {
    	USB_PR("%s: Can't find usb parameter\n", __func__);
        return;
    }

    USB_PR("vendor:%s,country:%s\n", usb_para_ptr->vender_para.vender_name, usb_para_ptr->vender_para.country_name);

    memcpy(&usb_para_data, usb_para_ptr, sizeof(smem_huawei_vender));
    
    /* decide usb pid array according to the vender name */
    if(!memcmp(usb_para_ptr->vender_para.vender_name, vender_name, strlen(vender_name)))
    {
        curr_usb_pid_ptr = &usb_pid_array[1];
        USB_PR("USB setting is TMO\n");
    }
    else
    {
        curr_usb_pid_ptr = &usb_pid_array[0];
        USB_PR("USB setting is NORMAL\n");
    }

    USB_PR("smem usb_serial=%s, usb_pid_index=%d\n", usb_para_ptr->usb_para.usb_serial, usb_para_ptr->usb_para.usb_pid_index);

    /* when manufacture, we need to use the diag. so if the usb_serial is null
       and the nv value is google index, we set the ports to normal.
    */
    if (0 == usb_para_data.usb_para.usb_serial[0] 
      && GOOGLE_INDEX == usb_para_ptr->usb_para.usb_pid_index)
    {
      USB_PR("%s usb serial number is null in google mode. so switch to original mode\n", __func__);
      usb_para_ptr->usb_para.usb_pid_index = ORI_INDEX;
    }

    usb_para_info.usb_pid_index = usb_para_ptr->usb_para.usb_pid_index;

    /* for debug */
    //usb_para_info.usb_pid_index = 22;
    
    usb_para_info.usb_pid = pid_index_to_pid(usb_para_ptr->usb_para.usb_pid_index);

    /* add new pid config for google */
    set_usb_pid_sn(usb_para_info.usb_pid_index);
    
    USB_PR("curr_usb_pid_ptr: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", 
        curr_usb_pid_ptr->cdrom_pid, 
        curr_usb_pid_ptr->norm_pid, 
        curr_usb_pid_ptr->udisk_pid,
        curr_usb_pid_ptr->auth_pid,
        curr_usb_pid_ptr->google_pid);
    USB_PR("usb_para_info: usb_pid_index=%d, usb_pid = 0x%x>\n", 
        usb_para_info.usb_pid_index, 
        usb_para_info.usb_pid);

}

#endif

static struct cy8c_ts_platform_data cy8ctma300_pdata = {
	.power_on = tma300_power,
	.dev_setup = tma300_dev_setup,
	.ts_name = "msm_tma300_ts",
	.dis_min_x = 0,
	.dis_max_x = 479,
	.dis_min_y = 0,
	.dis_max_y = 799,
	.res_x	 = 479,
	.res_y	 = 1009,
	.min_tid = 1,
	.max_tid = 255,
	.min_touch = 0,
	.max_touch = 255,
	.min_width = 0,
	.max_width = 255,
	.use_polling = 1,
	.invert_y = 1,
	.nfingers = 4,
};

static struct i2c_board_info cy8ctma300_board_info[] = {
	{
		I2C_BOARD_INFO("cy8ctma300", 0x2),
		.platform_data = &cy8ctma300_pdata,
	}
};
#ifdef CONFIG_HUAWEI_U8800PP1_WIFI_LOW_CONSUME
/*delete other powers except "wlan2"*/
void __init config_wifi_for_low_consume(void)
{
	
	int rc;
	struct vreg *vreg_wlan2 = NULL;
	vreg_wlan2 = vreg_get(NULL, "wlan2");
	if (IS_ERR(vreg_wlan2)) {
		printk(KERN_ERR "%s: vreg_get failed wlan2\n",
			__func__);
		return;
	}

	// Power up 2.5v Analog
	rc = vreg_set_level(vreg_wlan2, 2500);
	rc = vreg_enable(vreg_wlan2);
	
	msleep(10);

   //shut down this power after WIFI is initialised.   

	rc = vreg_disable(vreg_wlan2);
	if (rc) {
		printk(KERN_ERR "%s: vreg_disable failed vreg_wlan2\n",
			__func__);
	}
}
#endif
static void __init msm7x30_init(void)
{
	if (socinfo_init() < 0)
		printk(KERN_ERR "%s: socinfo_init() failed!\n",
		       __func__);
	msm_clock_init(msm_clocks_7x30, msm_num_clocks_7x30);
	platform_add_devices(early_devices, ARRAY_SIZE(early_devices));
#ifdef CONFIG_SERIAL_MSM_CONSOLE
	msm7x30_init_uart2();
#endif
	msm_spm_init(&msm_spm_data, 1);
	msm_acpu_clock_init(&msm7x30_clock_data);
	/* removed several lines */
#ifdef CONFIG_USB_FUNCTION
	msm_hsusb_pdata.swfi_latency =
		msm_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_hsusb_peripheral.dev.platform_data = &msm_hsusb_pdata;
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
#ifdef CONFIG_USB_GADGET
	msm_gadget_pdata.swfi_latency =
		msm_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
#endif
#endif
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(136);
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
	msm_device_tsif.dev.platform_data = &tsif_platform_data;
#endif
/* we must do platform_add_devices after we get init smem for usb */
    rmt_storage_add_ramfs();

	#ifdef CONFIG_HUAWEI_FEATURE_OEMINFO
    rmt_oeminfo_add_device();
	#endif

#ifdef CONFIG_HUAWEI_KERNEL
    hw_extern_sdcard_add_device();
#endif
#ifdef CONFIG_USB_EHCI_MSM
	msm_add_host(0, &msm_usb_host_pdata);
#endif
	msm7x30_init_mmc();
	/* removed several lines */
	msm_qsd_spi_init();
	msm_fb_add_devices();
	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	msm_device_i2c_init();
	msm_device_i2c_2_init();
	qup_device_i2c_init();
	buses_init();
	msm7x30_init_marimba();
#ifdef CONFIG_MSM7KV2_AUDIO
	snddev_poweramp_gpio_init();
	msm_snddev_init();
	aux_pcm_gpio_init();
#endif
#ifdef CONFIG_HUAWEI_FEATURE_VIBRATOR
	msm_init_pmic_vibrator();
#endif

	i2c_register_board_info(0, msm_i2c_board_info,
			ARRAY_SIZE(msm_i2c_board_info));

	if (!machine_is_msm8x55_svlte_ffa())
		marimba_pdata.tsadc = &marimba_tsadc_pdata;

	i2c_register_board_info(2, msm_marimba_board_info,
			ARRAY_SIZE(msm_marimba_board_info));

	i2c_register_board_info(4 /* QUP ID */, msm_camera_boardinfo,
				ARRAY_SIZE(msm_camera_boardinfo));
#ifdef CONFIG_HUAWEI_FEATURE_RIGHT_TPA2028D1_AMPLIFIER
	i2c_register_board_info(4 /* QUP ID */, msm_amplifier_boardinfo,
				ARRAY_SIZE(msm_amplifier_boardinfo));
#endif

	bt_power_init();
#ifdef CONFIG_I2C_SSBI
	msm_device_ssbi6.dev.platform_data = &msm_i2c_ssbi6_pdata;
	msm_device_ssbi7.dev.platform_data = &msm_i2c_ssbi7_pdata;
#endif
	if (machine_is_msm7x30_fluid() || machine_is_msm7x30_u8800())
		isa1200_init();

#if defined(CONFIG_TOUCHSCREEN_TSC2007) || \
	defined(CONFIG_TOUCHSCREEN_TSC2007_MODULE)
	if (machine_is_msm8x55_svlte_ffa())
		i2c_register_board_info(2, tsc_i2c_board_info,
				ARRAY_SIZE(tsc_i2c_board_info));
#endif

	if (machine_is_msm7x30_surf())
		platform_device_register(&flip_switch_device);
	pmic8058_leds_init();

#ifdef CONFIG_USB_AUTO_INSTALL
    proc_usb_para();
    platform_add_devices(devices, ARRAY_SIZE(devices));
#endif
	if (machine_is_msm7x30_fluid()) {
		/* Initialize platform data for fluid v2 hardware */
		if (SOCINFO_VERSION_MAJOR(
				socinfo_get_platform_version()) == 2) {
			cy8ctma300_pdata.res_y = 920;
			cy8ctma300_pdata.invert_y = 0;
			cy8ctma300_pdata.use_polling = 0;
			cy8ctma300_board_info[0].irq =
				MSM_GPIO_TO_INT(TS_GPIO_IRQ);
		}
		i2c_register_board_info(0, cy8ctma300_board_info,
			ARRAY_SIZE(cy8ctma300_board_info));
	}
	#ifdef CONFIG_HUAWEI_U8800PP1_WIFI_LOW_CONSUME
	config_wifi_for_low_consume();
	#endif
}

static unsigned pmem_sf_size = MSM_PMEM_SF_SIZE;
static void __init pmem_sf_size_setup(char **p)
{
	pmem_sf_size = memparse(*p, p);
}
__early_param("pmem_sf_size=", pmem_sf_size_setup);

static unsigned fb_size = MSM_FB_SIZE;
static void __init fb_size_setup(char **p)
{
	fb_size = memparse(*p, p);
}
__early_param("fb_size=", fb_size_setup);

static unsigned gpu_phys_size = MSM_GPU_PHYS_SIZE;
static void __init gpu_phys_size_setup(char **p)
{
	gpu_phys_size = memparse(*p, p);
}
__early_param("gpu_phys_size=", gpu_phys_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static void __init pmem_adsp_size_setup(char **p)
{
	pmem_adsp_size = memparse(*p, p);
}
__early_param("pmem_adsp_size=", pmem_adsp_size_setup);

static unsigned fluid_pmem_adsp_size = MSM_FLUID_PMEM_ADSP_SIZE;
static void __init fluid_pmem_adsp_size_setup(char **p)
{
	fluid_pmem_adsp_size = memparse(*p, p);
}
__early_param("fluid_pmem_adsp_size=", fluid_pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;
static void __init pmem_audio_size_setup(char **p)
{
	pmem_audio_size = memparse(*p, p);
}
__early_param("pmem_audio_size=", pmem_audio_size_setup);

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static void __init pmem_kernel_ebi1_size_setup(char **p)
{
	pmem_kernel_ebi1_size = memparse(*p, p);
}
__early_param("pmem_kernel_ebi1_size=", pmem_kernel_ebi1_size_setup);

static void __init msm7x30_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = pmem_sf_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for sf "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = fb_size ? : MSM_FB_SIZE;
	addr = alloc_bootmem(size);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));

	size = gpu_phys_size;
	if (size) {
		addr = alloc_bootmem(size);
		msm_kgsl_resources[1].start = __pa(addr);
		msm_kgsl_resources[1].end = msm_kgsl_resources[1].start + size - 1;
		pr_info("allocating %lu bytes at %p (%lx physical) for "
			"KGSL\n", size, addr, __pa(addr));
	}

	if machine_is_msm7x30_fluid()
		size = fluid_pmem_adsp_size;
	else
		size = pmem_adsp_size;

	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_audio_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_audio_pdata.start = __pa(addr);
		android_pmem_audio_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for audio "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}

}

static void __init msm7x30_map_io(void)
{
	msm_shared_ram_phys = 0x00100000;
	msm_map_msm7x30_io();
	msm7x30_allocate_memory_regions();
}

#define ATAG_CAMERA_ID 0x4d534D74
/* setup calls mach->fixup, then parse_tags, parse_cmdline
 * We need to setup meminfo in mach->fixup, so this function
 * will need to traverse each tag to find smi tag.
 */
int __init parse_tag_camera_id(const struct tag *tags)
{
    int camera_id = 0, find = 0;

	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_CAMERA_ID) {
			find = 1;
			break;
		}
	}
	if (find)
		camera_id = t->u.revision.rev;
	return camera_id;
}
__tagtable(ATAG_CAMERA_ID, parse_tag_camera_id);

#define ATAG_LCD_ID 0x4d534D73
int __init parse_tag_lcd_id(const struct tag *tags)
{
    int lcd_id = 0, find = 0;
	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_LCD_ID) {
			find = 1;
			break;
		}
	}
	if (find)
		lcd_id = t->u.revision.rev;
	return lcd_id;

}
__tagtable(ATAG_LCD_ID, parse_tag_lcd_id);

#define ATAG_TS_ID 0x4d534D75
int __init parse_tag_ts_id(const struct tag *tags)
{
    int ts_id = 0, find = 0;
	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_TS_ID) {
			find = 1;
			break;
		}
	}
	if (find)
		ts_id = t->u.revision.rev;
	return ts_id;

}
__tagtable(ATAG_TS_ID, parse_tag_ts_id);

#ifdef CONFIG_HUAWEI_POWER_DOWN_CHARGE
#define ATAG_CHARGE_FLAG  0x4d534D77
int __init parse_tag_charge_flag(const struct tag *tags)
{
    int charge_flag = 0, find = 0;
	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_CHARGE_FLAG) {
			find = 1;
			break;
		}
	}
	if (find)
		charge_flag = t->u.revision.rev;
	return charge_flag;

}
__tagtable(ATAG_CHARGE_FLAG, parse_tag_charge_flag);
#endif
#define ATAG_SUB_BOARD_ID 0x4d534D76
int __init parse_tag_sub_board_id(const struct tag *tags)
{
    int sub_board_id = 0, find = 0;
	struct tag *t = (struct tag *)tags;

	for (; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_SUB_BOARD_ID) {
			find = 1;
			break;
		}
	}
	if (find)
		sub_board_id = t->u.revision.rev;
	return sub_board_id;

}
__tagtable(ATAG_SUB_BOARD_ID, parse_tag_sub_board_id);
static void __init msm7x30_fixup(struct machine_desc *desc,
                                 struct tag *tags,
                                 char **cmdline,
                                 struct meminfo *mi)
{
    camera_id = parse_tag_camera_id((const struct tag *)tags);
    printk("%s:camera_id=%d\n", __func__, camera_id);
        
    lcd_id = parse_tag_lcd_id((const struct tag *)tags);
    printk("%s:lcd_id=%d\n", __func__, lcd_id);

    ts_id = parse_tag_ts_id((const struct tag *)tags);
    printk("%s:ts_id=%d\n", __func__, ts_id);

    sub_board_id = parse_tag_sub_board_id((const struct tag *)tags);
    printk("%s:sub_board_id=%d\n", __func__, sub_board_id);

#ifdef CONFIG_HUAWEI_POWER_DOWN_CHARGE    
    charge_flag = parse_tag_charge_flag((const struct tag *)tags);
    printk("%s:charge_flag=%d\n", __func__, charge_flag);
#endif
    
}

hw_ver_sub_type get_hw_sub_board_id(void)
{
    return (hw_ver_sub_type)(sub_board_id&HW_VER_SUB_MASK);
}

#ifdef CONFIG_HUAWEI_POWER_DOWN_CHARGE
unsigned int get_charge_flag(void)
{
    return charge_flag;
}
#endif

MACHINE_START(MSM7X30_SURF, "QCT MSM7X30 SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(MSM7X30_FFA, "QCT MSM7X30 FFA")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(MSM7X30_FLUID, "QCT MSM7X30 FLUID")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(MSM8X55_SURF, "QCT MSM8X55 SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(MSM8X55_FFA, "QCT MSM8X55 FFA")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END
MACHINE_START(MSM8X55_SVLTE_SURF, "QCT MSM8X55 SVLTE SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END
MACHINE_START(MSM8X55_SVLTE_FFA, "QCT MSM8X55 SVLTE FFA")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(MSM7X30_U8800, "u8800")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.fixup = msm7x30_fixup,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END
MACHINE_START(MSM7X30_U8820, "HUAWEI U8820 BOARD")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.fixup = msm7x30_fixup,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END
