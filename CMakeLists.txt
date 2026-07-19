target_sources_ifdef(CONFIG_ZMK_BATTERY_NRF_VDDH_CURVE
                     app PRIVATE src/sensors/battery_nrf_vddh_curve.c)

if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_MOMENTARY_LAYER_LOCK
                       app PRIVATE src/behaviors/behavior_momentary_layer_lock.c)
endif()
