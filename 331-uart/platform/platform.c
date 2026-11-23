#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define LED_PIN 25
#define BUTTON_PIN 15
#define LINE_PIN 22
#define ADC_PIN 26
#define ADC_CHANNEL 0
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096u

// выводы модуля UART0
#define UART0_TX_PIN 0
#define UART0_RX_PIN 1

#define UART1_TX_PIN 8
#define UART1_RX_PIN 9

#define UART_BAUD 115200

// У каждого приёмника свой итог, чтобы ошибка раннего байта не потерялась.
// К признакам может обращаться обработчик прерывания UART0.
static volatile uint32_t rx_errors[2];

// Буфер длиннее аппаратной FIFO (32 байта), вмещает весь блок из 64 байтов.
#define RX_BUFFER_SIZE 128u
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint32_t rx_count;

// Считываем все готовые элементы FIFO без ожидания новых байтов.
static void on_uart0_rx(void)
{
    while (uart_is_readable(uart0))
    {
        const uint32_t sample = uart_get_hw(uart0)->dr;
        // Сохраняем FE/PE/BE из байтов; OE читаем из UARTRSR.
        rx_errors[PLATFORM_UART0] |=
            (sample >> UART_UARTDR_FE_LSB) &
            (PLATFORM_UART_FE | PLATFORM_UART_PE | PLATFORM_UART_BE);

        if (rx_count < RX_BUFFER_SIZE)
        {
            rx_buffer[rx_count++] = (uint8_t)(sample & UART_UARTDR_DATA_BITS);
        }
    }
}

static uart_inst_t *uart_of(platform_uart_t uart)
{
    return uart == PLATFORM_UART0 ? uart0 : uart1;
}

void platform_init(void)
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    adc_init();
    adc_gpio_init(ADC_PIN);

    gpio_init(LINE_PIN);
    platform_line_set(PLATFORM_LINE_PUSH_PULL, false);

    // делитель, формат 8N1, очереди, включение модуля
    uart_init(uart0, UART_BAUD);
    platform_uart_loop_set(false);

    // uart_init включает FIFO и 8N1. Выводы UART1 всегда ведут наружу.
    uart_init(uart1, UART_BAUD);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);

    // IRQ разрешён в контроллере, но сам UART0 запросит его лишь в режиме block irq.
    irq_set_exclusive_handler(UART0_IRQ, on_uart0_rx);
    irq_set_enabled(UART0_IRQ, true);
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_button_read(void)
{
    return gpio_get(BUTTON_PIN) == 0;
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return (float)adc_read() * (ADC_VREF_V / ADC_LEVELS);
}

void platform_line_set(platform_line_mode_t mode, bool high)
{
    if (mode == PLATFORM_LINE_PUSH_PULL)
    {
        gpio_disable_pulls(LINE_PIN);
        gpio_put(LINE_PIN, high);
        gpio_set_dir(LINE_PIN, GPIO_OUT);
        return;
    }

    // Регистр выхода всегда содержит 0. Вход отключает сильный драйвер.
    gpio_pull_up(LINE_PIN);
    gpio_put(LINE_PIN, 0);
    gpio_set_dir(LINE_PIN, high ? GPIO_IN : GPIO_OUT);
}

void platform_uart_loop_set(bool internal)
{
    uint function = internal ? GPIO_FUNC_NULL : GPIO_FUNC_UART;
    gpio_set_function(UART0_TX_PIN, function);
    gpio_set_function(UART0_RX_PIN, function);

    if (internal)
    {
        hw_set_bits(&uart_get_hw(uart0)->cr, UART_UARTCR_LBE_BITS);
    }
    else
    {
        hw_clear_bits(&uart_get_hw(uart0)->cr, UART_UARTCR_LBE_BITS);
    }
}

void platform_uart_rx_irq_set(bool on)
{
    // SDK разрешает IRQ по уровню заполнения RX FIFO и по таймауту приёма.
    uart_set_irqs_enabled(uart0, on, false);
}

void platform_uart_clear(platform_uart_t uart)
{
    uart_inst_t *u = uart_of(uart);

    while (uart_is_readable(u))
    {
        (void)uart_get_hw(u)->dr;
    }

    // Очистка UARTECR: любая запись сбрасывает FE/PE/BE/OE.
    // OE для нового блока получаем из UARTRSR (не из UARTDR).
    uart_get_hw(u)->rsr = 0;
    rx_errors[uart] = 0;
    if (uart == PLATFORM_UART0)
    {
        rx_count = 0;
    }
}

bool platform_uart_put(platform_uart_t uart, uint8_t byte)
{
    uart_inst_t *u = uart_of(uart);

    if (!uart_is_writable(u))
    {
        return false;
    }

    // место в очереди есть, поэтому uart_putc_raw() не ждёт
    uart_putc_raw(u, byte);
    return true;
}

bool platform_uart_busy(platform_uart_t uart)
{
    return uart_get_hw(uart_of(uart))->fr & UART_UARTFR_BUSY_BITS;
}

size_t platform_uart_read(platform_uart_t uart, uint8_t *data, size_t size)
{
    uart_inst_t *u = uart_of(uart);
    size_t n = 0;

    // При включённом IRQ байты уже извлечены из FIFO обработчиком.
    // uart-task вызывает эту функцию, когда линия молчит 1 мс.
    if (uart == PLATFORM_UART0)
    {
        const uint32_t count = rx_count;
        while (n < count && n < size)
        {
            data[n] = rx_buffer[n];
            n++;
        }
    }

    // В режиме poll байты остаются в аппаратной FIFO.
    while (uart_is_readable(u) && n < size)
    {
        // Одно чтение DR вынимает один элемент FIFO: байт и его признаки.
        uint32_t sample = uart_get_hw(u)->dr;
        data[n++] = (uint8_t)(sample & UART_UARTDR_DATA_BITS);
        // OE берём только из UARTRSR, чтобы не переносить ошибку
        // переполнения предыдущего блока через UARTDR.
        rx_errors[uart] |= (sample >> UART_UARTDR_FE_LSB) &
            (PLATFORM_UART_FE | PLATFORM_UART_PE | PLATFORM_UART_BE);
    }

    return n;
}

uint32_t platform_uart_errors(platform_uart_t uart)
{
    return rx_errors[uart] | (uart_get_hw(uart_of(uart))->rsr & UART_UARTRSR_BITS);
}

uint32_t platform_uart_baud_set(platform_uart_t uart, uint32_t baud)
{
    return uart_set_baudrate(uart_of(uart), baud);
}

void platform_uart_parity_set(platform_uart_t uart, platform_parity_t parity)
{
    static const uart_parity_t parities[] = {
        UART_PARITY_NONE,
        UART_PARITY_EVEN,
        UART_PARITY_ODD,
    };

    uart_set_format(uart_of(uart), 8, 1, parities[parity]);
}
