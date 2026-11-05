/* Set UICR.REGOUT0 to 3.3 V once, the same way the Adafruit nRF52 bootloader
 * does for nice!nano-style boards. After that, report the value on the
 * onboard RGB LED: green = 3.3 V, red = anything else. */
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <hal/nrf_gpio.h>
#include <nrfx.h>

#define VOUT_FIELD(v) ((v) << UICR_REGOUT0_VOUT_Pos)

static void nvmc_wait(void)
{
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
	}
}

static int regout0_set(void)
{
	uint32_t cur = NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk;

	if (cur != VOUT_FIELD(UICR_REGOUT0_VOUT_DEFAULT)) {
		return 0; /* already programmed: never touch it again */
	}
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
	nvmc_wait();
	NRF_UICR->REGOUT0 = (NRF_UICR->REGOUT0 & ~UICR_REGOUT0_VOUT_Msk) |
			    VOUT_FIELD(UICR_REGOUT0_VOUT_3V3);
	nvmc_wait();
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
	nvmc_wait();
	/* UICR takes effect after reset; reset only if the write stuck, so a
	 * failed write cannot cause a boot loop. */
	if ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
	    VOUT_FIELD(UICR_REGOUT0_VOUT_3V3)) {
		NVIC_SystemReset();
	}
	return 0;
}
SYS_INIT(regout0_set, PRE_KERNEL_1, 0);

/* RGB LED, common anode: low = on. Red P0.26, green P0.30. */
#define LED_RED   NRF_GPIO_PIN_MAP(0, 26)
#define LED_GREEN NRF_GPIO_PIN_MAP(0, 30)

static int regout0_report(void)
{
	bool ok = (NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
		  VOUT_FIELD(UICR_REGOUT0_VOUT_3V3);
	uint32_t pin = ok ? LED_GREEN : LED_RED;

	nrf_gpio_cfg_output(pin);
	nrf_gpio_pin_clear(pin);
	k_msleep(2000);
	nrf_gpio_cfg_default(pin); /* back to disconnected input */
	return 0;
}
SYS_INIT(regout0_report, APPLICATION, 99);
