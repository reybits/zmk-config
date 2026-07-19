/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Vendored from upstream ZMK's battery_nrf_vddh.c (app/module/drivers/
 * sensor/battery, revision 484a0547) with two local changes:
 *
 *  - millivolts are converted to a percentage by piecewise-linear
 *    interpolation over a 1S LiPo rest-discharge curve instead of
 *    upstream's single straight line, which overstates the low end of
 *    the scale by 20+ points (upstream PRs #611 and #2066 tried to fix
 *    this and both stalled);
 *  - the ADC is sampled three times and the median is used, so a
 *    reading taken during a BLE TX or display-refresh voltage sag does
 *    not jitter the reported percentage.
 *
 * Delete this file (plus the &vbatt override in the rolio shield
 * overlay and the Kconfig/CMake glue) if upstream ever ships
 * configurable discharge curves.
 */

#define DT_DRV_COMPAT zmk_battery_nrf_vddh_curve

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define VDDHDIV (5)
#define VDDH_SAMPLES (3)

// Inlined from upstream's battery_common.h/.c: that header is private to
// the zmk module's sensor library, so this driver carries its own copy.
struct battery_value {
    uint16_t adc_raw;
    uint16_t millivolts;
    uint8_t state_of_charge;
};

static int battery_channel_get(const struct battery_value *value, enum sensor_channel chan,
                               struct sensor_value *val_out) {
    switch (chan) {
    case SENSOR_CHAN_GAUGE_VOLTAGE:
        val_out->val1 = value->millivolts / 1000;
        val_out->val2 = (value->millivolts % 1000) * 1000U;
        break;

    case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
        val_out->val1 = value->state_of_charge;
        val_out->val2 = 0;
        break;

    default:
        return -ENOTSUP;
    }

    return 0;
}

struct curve_point {
    int16_t millivolts;
    int8_t percent;
};

// 1S LiPo rest-discharge anchors from the ampow.com chart (the source
// upstream PR #2066 cited). Percentage is interpolated linearly between
// adjacent anchors.
static const struct curve_point lipo_curve[] = {
    {4200, 100}, {4150, 95}, {4110, 90}, {4080, 85}, {4020, 80}, {3980, 75}, {3950, 70},
    {3910, 65},  {3870, 60}, {3850, 55}, {3840, 50}, {3820, 45}, {3800, 40}, {3790, 35},
    {3770, 30},  {3750, 25}, {3730, 20}, {3710, 15}, {3690, 10}, {3610, 5},  {3270, 0},
};

static uint8_t lipo_mv_to_pct(int32_t mv) {
    if (mv >= lipo_curve[0].millivolts) {
        return lipo_curve[0].percent;
    }

    for (size_t i = 1; i < ARRAY_SIZE(lipo_curve); i++) {
        const struct curve_point *upper = &lipo_curve[i - 1];
        const struct curve_point *lower = &lipo_curve[i];
        if (mv >= lower->millivolts) {
            return lower->percent + (int32_t)(upper->percent - lower->percent) *
                                        (mv - lower->millivolts) /
                                        (upper->millivolts - lower->millivolts);
        }
    }

    return 0;
}

static const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(adc));

struct vddh_data {
    struct adc_channel_cfg acc;
    struct adc_sequence as;
    struct battery_value value;
};

static int vddh_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    // Make sure selected channel is supported
    if (chan != SENSOR_CHAN_GAUGE_VOLTAGE && chan != SENSOR_CHAN_GAUGE_STATE_OF_CHARGE &&
        chan != SENSOR_CHAN_ALL) {
        LOG_DBG("Selected channel is not supported: %d.", chan);
        return -ENOTSUP;
    }

    struct vddh_data *drv_data = dev->data;
    struct adc_sequence *as = &drv_data->as;

    int32_t mv[VDDH_SAMPLES];

    for (int i = 0; i < VDDH_SAMPLES; i++) {
        int rc = adc_read(adc, as);
        as->calibrate = false;

        if (rc != 0) {
            LOG_ERR("Failed to read ADC: %d", rc);
            return rc;
        }

        int32_t val = drv_data->value.adc_raw;
        rc = adc_raw_to_millivolts(adc_ref_internal(adc), drv_data->acc.gain, as->resolution,
                                   &val);
        if (rc != 0) {
            LOG_ERR("Failed to convert raw ADC to mV: %d", rc);
            return rc;
        }

        mv[i] = val * VDDHDIV;
    }

    const int32_t median = MAX(MIN(mv[0], mv[1]), MIN(MAX(mv[0], mv[1]), mv[2]));

    drv_data->value.millivolts = median;
    drv_data->value.state_of_charge = lipo_mv_to_pct(median);

    LOG_DBG("VDDH median %d mV => %d%%", median, drv_data->value.state_of_charge);

    return 0;
}

static int vddh_channel_get(const struct device *dev, enum sensor_channel chan,
                            struct sensor_value *val) {
    struct vddh_data const *drv_data = dev->data;
    return battery_channel_get(&drv_data->value, chan, val);
}

static const struct sensor_driver_api vddh_api = {
    .sample_fetch = vddh_sample_fetch,
    .channel_get = vddh_channel_get,
};

static int vddh_init(const struct device *dev) {
    struct vddh_data *drv_data = dev->data;

    if (!device_is_ready(adc)) {
        LOG_ERR("ADC device is not ready %s", adc->name);
        return -ENODEV;
    }

    drv_data->as = (struct adc_sequence){
        .channels = BIT(0),
        .buffer = &drv_data->value.adc_raw,
        .buffer_size = sizeof(drv_data->value.adc_raw),
        .oversampling = 4,
        .calibrate = true,
    };

#ifdef CONFIG_ADC_NRFX_SAADC
    drv_data->acc = (struct adc_channel_cfg){
        .gain = ADC_GAIN_1_2,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
        .input_positive = SAADC_CH_PSELN_PSELN_VDDHDIV5,
    };

    drv_data->as.resolution = 12;
#else
#error Unsupported ADC
#endif

    const int rc = adc_channel_setup(adc, &drv_data->acc);
    LOG_DBG("VDDHDIV5 setup returned %d", rc);

    return rc;
}

static struct vddh_data vddh_data;

DEVICE_DT_INST_DEFINE(0, &vddh_init, NULL, &vddh_data, NULL, POST_KERNEL,
                      CONFIG_SENSOR_INIT_PRIORITY, &vddh_api);
