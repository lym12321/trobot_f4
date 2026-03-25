//
// Created by fish on 2025/9/26.
//

#include "bsp/led.h"

#include <math.h>

#include "tim.h"

void bsp_led_init() {
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);
}

void bsp_led_set(uint8_t r, uint8_t g, uint8_t b) {
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_1, b);
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_2, g);
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_3, r);
}

void bsp_led_set_hsv(float h, float s, float v) {
    h = fmodf(h, 1.0f) * 6.0f;

    float f = h - floorf(h), p = v * (1 - s), q = v * (1 - s * f), t = v * (1 - s * (1 - f));

    float r,g,b;
    switch((int) (floorf(h)) % 6){
        case 0: r=v; g=t; b=p; break;
        case 1: r=q; g=v; b=p; break;
        case 2: r=p; g=v; b=t; break;
        case 3: r=p; g=q; b=v; break;
        case 4: r=t; g=p; b=v; break;
        default:r=v; g=p; b=q; break;
    }

    bsp_led_set((uint8_t) (r*255+0.5f), (uint8_t) (g*255+0.5f), (uint8_t) (b*255+0.5f));
}