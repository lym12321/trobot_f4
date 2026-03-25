//
// Created by fish on 2025/9/24.
//

#include "bsp/can.h"

#include <string.h>

#include "can.h"

static bsp_can_handle_t *handle[BSP_CAN_DEVICE_COUNT] = {
    [E_CAN_1] = &hcan1,
    [E_CAN_2] = &hcan2
};

static uint8_t cnt[BSP_CAN_DEVICE_COUNT][2];
static uint32_t pkg_id[BSP_CAN_DEVICE_COUNT][2][BSP_CAN_FILTER_LIMIT];
static bsp_can_callback_t callback[BSP_CAN_DEVICE_COUNT][2][BSP_CAN_FILTER_LIMIT];

_ram_d1 static uint8_t rx_buffer[BSP_CAN_DEVICE_COUNT][BSP_CAN_FILTER_LIMIT][BSP_CAN_BUFFER_SIZE];

void bsp_can_init(bsp_can_e device) {
    HAL_CAN_Start(handle[device]);
    HAL_CAN_ActivateNotification(handle[device], CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_ActivateNotification(handle[device], CAN_IT_RX_FIFO1_MSG_PENDING);
}

static uint8_t can_filter_idx[2] = { 0, 14 };

void bsp_can_set_callback(bsp_can_e device, uint32_t id, bsp_can_callback_t func) {
    const uint8_t is_ext = id > 0x7ff;
    BSP_ASSERT(cnt[device][is_ext] < BSP_CAN_FILTER_LIMIT && func != NULL);
    pkg_id[device][is_ext][cnt[device][is_ext]] = id;
    callback[device][is_ext][cnt[device][is_ext]] = func;

    CAN_FilterTypeDef filter = {
        .FilterMode = CAN_FILTERMODE_IDLIST,
        .FilterScale = CAN_FILTERSCALE_16BIT,
        .FilterFIFOAssignment = (id & 1) ? CAN_RX_FIFO0 : CAN_RX_FIFO1,
        .SlaveStartFilterBank = 14,
        .FilterIdLow = id << 5,
        .FilterBank = can_filter_idx[device == E_CAN_2] ++,
        .FilterActivation = CAN_FILTER_ENABLE
    };

    BSP_ASSERT(HAL_CAN_ConfigFilter(handle[device], &filter) == HAL_OK);

    cnt[device][is_ext] ++;
}

static uint32_t len2code(uint8_t l) {
    if(l <= 8) return l;
    BSP_ASSERT(0); return 0;
}

// len <= 8 时使用标准 can，len > 8 时使用 fdcan
// 同一 can 总线上不可同时出现标准 can 和 fdcan 两种数据包
void bsp_can_send(bsp_can_e device, uint32_t id, const uint8_t *data, uint8_t len) {
    BSP_ASSERT(data != NULL && len > 0 && len <= 64);
    CAN_TxHeaderTypeDef header = {
        .StdId = id,
        .IDE = id <= 0x7ff ? CAN_ID_STD : CAN_ID_EXT,
        .RTR = CAN_RTR_DATA,
        .DLC = len2code(len)
    };
    uint32_t tx_mailbox;
    HAL_CAN_AddTxMessage(handle[device], &header, data, &tx_mailbox);
}

static uint8_t code2len(uint32_t l) {
    if(l <= 8) return l;
    BSP_ASSERT(0); return 0;
}

void bsp_can_callback_sol(bsp_can_e device, uint32_t fifo) {
    CAN_RxHeaderTypeDef header;
    uint8_t buf[BSP_CAN_BUFFER_SIZE] = { 0 };
    while (HAL_CAN_GetRxFifoFillLevel(handle[device], fifo)) {
        if (HAL_CAN_GetRxMessage(handle[device], fifo, &header, buf) != HAL_OK) break;
        uint8_t is_ext = header.IDE == CAN_ID_EXT;
        for (uint8_t i = 0; i < cnt[device][is_ext]; i++) {
            if (pkg_id[device][is_ext][i] == header.StdId) {
                uint8_t len = code2len(header.DLC);
                memcpy(rx_buffer[device][i], buf, len);
                if (callback[device][is_ext][i] != NULL) callback[device][is_ext][i](device, header.StdId, rx_buffer[device][i], len);
                break;
            }
        }
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *device) {
    for(uint8_t i = 0; i < BSP_CAN_DEVICE_COUNT; i++) {
        if(handle[i] == device) {
            bsp_can_callback_sol(i, CAN_RX_FIFO0);
            break;
        }
    }
}

void HAL_CAN_RxFif10MsgPendingCallback(CAN_HandleTypeDef *device) {
    for(uint8_t i = 0; i < BSP_CAN_DEVICE_COUNT; i++) {
        if(handle[i] == device) {
            bsp_can_callback_sol(i, CAN_RX_FIFO1);
            break;
        }
    }
}