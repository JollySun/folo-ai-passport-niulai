#pragma once

#include "esp_err.h"

typedef enum {
    BSP_BTN_UP = 0,
    BSP_BTN_DOWN,
    BSP_BTN_OK,
} bsp_btn_t;

typedef enum {
    BSP_BTN_PRESS = 0,
    BSP_BTN_RELEASE,
    BSP_BTN_CLICK,
    BSP_BTN_DOUBLE,
    BSP_BTN_LONG,
} bsp_btn_ev_t;

typedef void (*bsp_btn_cb_t)(bsp_btn_t button, bsp_btn_ev_t event, void *user);

esp_err_t bsp_button_init(bsp_btn_cb_t callback, void *user);
