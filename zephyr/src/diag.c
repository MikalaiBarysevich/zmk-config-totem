/* power-bisect only: print what the chip says about itself, so the excess
 * current can be checked against silicon revision and regulator mode. */
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/dt-bindings/adc/nrf-adc.h>
#include <zephyr/logging/log.h>
#include <nrfx.h>

LOG_MODULE_REGISTER(totem_diag, LOG_LEVEL_INF);

static const struct device *const adc = DEVICE_DT_GET(DT_NODELABEL(adc));

/* Channels 5/6 are free; ZMK's battery divider uses channel 7. */
static int read_mv(uint8_t ch, uint8_t input, int32_t *mv)
{
	struct adc_channel_cfg cfg = {
		.gain = ADC_GAIN_1_6,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
		.channel_id = ch,
		.input_positive = input,
	};
	int16_t raw = 0;
	struct adc_sequence seq = {
		.channels = BIT(ch),
		.buffer = &raw,
		.buffer_size = sizeof(raw),
		.resolution = 12,
		.oversampling = 4,
	};
	int err = adc_channel_setup(adc, &cfg);

	if (!err) {
		err = adc_read(adc, &seq);
	}
	if (err) {
		return err;
	}
	*mv = raw;
	return adc_raw_to_millivolts(adc_ref_internal(adc), ADC_GAIN_1_6, 12, mv);
}

static void diag(void)
{
	uint32_t v = NRF_FICR->INFO.VARIANT;
	char var[5] = {v >> 24, v >> 16, v >> 8, v, 0};
	uint32_t resetreas = NRF_POWER->RESETREAS;

	k_sleep(K_SECONDS(3));
	for (;;) {
		int32_t vdd = -1, vddh = -1;

		read_mv(5, NRF_SAADC_VDD, &vdd);
		read_mv(6, NRF_SAADC_VDDHDIV5, &vddh);
		LOG_INF("PART=%x VARIANT=%s PACKAGE=%x RAM=%u FLASH=%u",
			NRF_FICR->INFO.PART, var, NRF_FICR->INFO.PACKAGE,
			NRF_FICR->INFO.RAM, NRF_FICR->INFO.FLASH);
		LOG_INF("DEVICEID=%08x%08x", NRF_FICR->DEVICEID[1],
			NRF_FICR->DEVICEID[0]);
		LOG_INF("UICR.REGOUT0=%08x MAINREGSTATUS=%u DCDCEN=%u DCDCEN0=%u",
			NRF_UICR->REGOUT0, NRF_POWER->MAINREGSTATUS,
			NRF_POWER->DCDCEN, NRF_POWER->DCDCEN0);
		LOG_INF("USBREGSTATUS=%08x RESETREAS(boot)=%08x HFCLKSTAT=%08x LFCLKSTAT=%08x",
			NRF_POWER->USBREGSTATUS, resetreas,
			NRF_CLOCK->HFCLKSTAT, NRF_CLOCK->LFCLKSTAT);
		LOG_INF("VDD=%d mV VDDH=%d mV", vdd, vddh * 5);
		k_sleep(K_SECONDS(5));
	}
}

K_THREAD_DEFINE(totem_diag_tid, 1024, diag, NULL, NULL, NULL, 14, 0, 0);
