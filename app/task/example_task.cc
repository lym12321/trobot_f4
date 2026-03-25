//
// Created by fish on 2026/1/24.
//

#include "bsp/time.h"
#include "utils/os.h"
#include "utils/logger.h"

[[noreturn]] void example_task(void *args) {
    auto lst_wkup = bsp_time_get_ms();

    logger::info("task inited");

    for (;;) {
        logger::info("task running: %d", bsp_time_get_ms());
        os::task::sleep_until_ms(1, lst_wkup);
    }
}
