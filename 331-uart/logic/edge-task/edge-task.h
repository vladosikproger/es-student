#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "platform/platform.h"

// Время до порогов — в микросекундах. При тайм-ауте ok == false.
typedef void (*edge_task_callback_t)(platform_line_mode_t mode, bool ok,
                                     uint32_t rise_us, uint32_t fall_us);

// false, если ещё идёт предыдущий замер.
bool edge_task_start(platform_line_mode_t mode, edge_task_callback_t on_done);
void edge_task_handle(void);
