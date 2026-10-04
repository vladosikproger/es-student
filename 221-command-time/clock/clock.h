#ifndef CLOCK_H
#define CLOCK_H

void clk_info(void);
void uptime(void);

// Открытые функции-шлюзы для управления частотой
void clk_sys_low(void);
void clk_sys_default(void);

#endif