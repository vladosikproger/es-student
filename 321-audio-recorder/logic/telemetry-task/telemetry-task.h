// Задача телеметрии: периодически измеряет напряжение и температуру,
// а готовую запись передаёт функции, зарегистрированной при запуске.
// Способ доставки записи (USB, UART, файл и т. п.) задаче неизвестен.
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    TELEMETRY_STATE_IDLE,
    TELEMETRY_STATE_RUN,
} telemetry_state_t;

// Один набор измерений с временной меткой системного таймера.
typedef struct
{
    uint64_t time_us;
    float voltage_v;
    float temp_c;
} telemetry_record_t;

// Callback: функция получает адрес неизменяемой записи и ничего не возвращает.
typedef void (*telemetry_task_callback_t)(const telemetry_record_t *record);

void telemetry_task_init(telemetry_task_callback_t on_record);

// Обработчик вызывается на каждой итерации суперцикла и не ждёт истечения периода.
void telemetry_task_handle(void);

// Возвращает false, если период меньше минимально допустимых 10 мс.
bool telemetry_task_start(uint32_t period_ms);
void telemetry_task_stop(void);

telemetry_state_t telemetry_task_get_state(void);
uint32_t telemetry_task_get_period_ms(void);
