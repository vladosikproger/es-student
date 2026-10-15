#include "pi-task.h"
#include "profiling/profiling.h"
#include "systime/systime.h" // Подключаем независимого диспетчера времени

#define PI_TERMS_PER_CALL 100 

static pi_state_t state;
static uint32_t pi_k;
static uint32_t pi_n;
static double pi_sum;
static double pi_sign;

static uint64_t start_time_us;
static uint64_t end_time_us;

void pi_task_init(void)
{
    state = PI_STATE_NOT_STARTED;
}

bool pi_task_start(uint32_t terms)
{
    if (state == PI_STATE_RUNNING) {
        return false; 
    }

    pi_n = terms;
    pi_k = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;
    
    // ОБРАЩАЕМСЯ К ДИСПЕТЧЕРУ (вместо прямого обращения к железу)
    start_time_us = systime_us();
    state = PI_STATE_RUNNING;
    
    return true;
}

void pi_task_handle(void)
{
    if (state != PI_STATE_RUNNING) {
        return;
    }

    uint32_t end = pi_k + PI_TERMS_PER_CALL;
    
    if (end < pi_k || end > pi_n) {
        end = pi_n;
    }

    for (; pi_k < end; pi_k++) {
        pi_sum += pi_sign / (2.0 * pi_k + 1.0);
        pi_sign = -pi_sign;
    }

    if (pi_k == pi_n) {
        // ОБРАЩАЕМСЯ К ДИСПЕТЧЕРУ (вместо прямого обращения к железу)
        end_time_us = systime_us(); 
        state = PI_STATE_READY;
    }
}

pi_state_t pi_task_get_result(double *result, uint32_t *time_ms)
{
    if (state == PI_STATE_READY) {
        *result = 4.0 * pi_sum;
        *time_ms = (end_time_us - start_time_us) / 1000; 
    }
    return state;
}