#include "platform/platform.h"
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/time.h"

#include <stddef.h>

#define LED_PIN 25
#define BUTTON_1_PIN 15
#define BUTTON_2_PIN 14
#define CHARGE_PIN 22

#define ADC_PIN 26
#define ADC_CHANNEL 0
#define ADC_TEMP_CHANNEL 4
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096
#define ADC_TEMP_V_AT_27_C 0.706f
#define ADC_TEMP_V_PER_C 0.001721f

static const uint32_t button_pins[PLATFORM_BUTTON_COUNT] =
{
    BUTTON_1_PIN,
    BUTTON_2_PIN
};

// Кто сейчас пользуется единственным блоком АЦП RP2040.
typedef enum
{
    ADC_FREE,
    ADC_CAPTURE,
    ADC_BLOCK,
    ADC_STREAM,
} adc_user_t;

static volatile adc_user_t adc_user;

#define ADC_CLOCK_HZ 48000000.0f

static int block_channel;
static uint16_t block_samples[PLATFORM_BLOCK_MAX];
static uint32_t block_start_us;
static volatile uint32_t block_end_us;
static volatile bool block_ready;

// Два канала DMA по очереди заполняют два буфера потока.
static int stream_channels[2];
static uint8_t stream_buffers[2][PLATFORM_STREAM_BYTES] __attribute__((aligned(4)));
static volatile bool stream_ready[2];
static volatile uint32_t stream_overruns;
static uint32_t stream_samples;
static uint32_t stream_next;

// Возвращает АЦП в режим одиночных измерений, используемый get_voltage,
// get_temp и телеметрией.
static void adc_one_shot(void)
{
    adc_run(false);
    adc_fifo_drain();
    adc_fifo_setup(false, false, 0, false, false);
    adc_set_clkdiv(0);
}

// Обработчик DMA_IRQ_1: DMA перенёс последний отсчёт блока в RAM.
// Здесь выполняются только короткие действия; массив обрабатывается в logic.
static void on_block_done(void)
{
    dma_channel_acknowledge_irq1(block_channel);
    block_end_us = time_us_32();
    adc_one_shot();
    block_ready = true;
    adc_user = ADC_FREE;
}

// Обработчик DMA_IRQ_0: канал закончил буфер; следующий канал уже запущен
// аппаратной цепочкой CHAIN_TO. Здесь нельзя печатать или отправлять кадры.
static void on_stream_buffer(void)
{
    for (int i = 0; i < 2; ++i)
    {
        if (dma_channel_get_irq0_status((uint)stream_channels[i]))
        {
            dma_channel_acknowledge_irq0((uint)stream_channels[i]);

            // DMA дошёл до конца массива; при следующем запуске писать с начала.
            dma_channel_set_write_addr((uint)stream_channels[i], stream_buffers[i], false);

            // Пока этот канал работал, следующий буфер должен был быть отправлен.
            // Если он всё ещё готов, DMA сейчас начнёт перезаписывать неотправленные данные.
            if (stream_ready[(i + 1) % 2])
            {
                stream_overruns++;
            }
            stream_ready[i] = true;
        }
    }
}

void platform_init(void)
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Обе кнопки используют внутренние подтягивающие резисторы.
    // В отпущенном состоянии GPIO = 1; при нажатии кнопка соединяет GPIO с GND.
    for (uint32_t i = 0; i < (uint32_t)PLATFORM_BUTTON_COUNT; ++i)
    {
        gpio_init(button_pins[i]);
        gpio_set_dir(button_pins[i], GPIO_IN);
        gpio_pull_up(button_pins[i]);
    }

    // GP22 — оставшийся вывод прежней RC-практики; безопасное начальное состояние LOW.
    gpio_init(CHARGE_PIN);
    gpio_set_dir(CHARGE_PIN, GPIO_OUT);
    gpio_put(CHARGE_PIN, 0);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_set_temp_sensor_enabled(true);

    // Канал блока использует DMA_IRQ_1; DMA_IRQ_0 остаётся для следующего задания.
    block_channel = dma_claim_unused_channel(true);
    dma_channel_set_irq1_enabled(block_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_1, on_block_done);
    irq_set_enabled(DMA_IRQ_1, true);

    // Два канала потока сообщают о завершении буферов через DMA_IRQ_0.
    // Их прерывания на уровне каналов разрешаются только при stream_start.
    stream_channels[0] = (int)dma_claim_unused_channel(true);
    stream_channels[1] = (int)dma_claim_unused_channel(true);
    irq_set_exclusive_handler(DMA_IRQ_0, on_stream_buffer);
    irq_set_enabled(DMA_IRQ_0, true);
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_button_read(platform_button_t button)
{
    const uint32_t index = (uint32_t)button;
    if (index >= (uint32_t)PLATFORM_BUTTON_COUNT)
    {
        return false;
    }

    return gpio_get(button_pins[index]) == 0;
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return (float)adc_read() * ADC_VREF_V / ADC_LEVELS;
}

float platform_temp_read(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);
    const float voltage = (float)adc_read() * ADC_VREF_V / ADC_LEVELS;
    return 27.0f - (voltage - ADC_TEMP_V_AT_27_C) / ADC_TEMP_V_PER_C;
}

void platform_charge_set(bool on)
{
    gpio_put(CHARGE_PIN, on);
}

// Объект должен жить всё время, пока SDK обслуживает повторяющийся будильник.
static repeating_timer_t blink_timer;

// Эти данные читаются/изменяются из разных контекстов:
// основной код управляет запуском/остановкой, callback вызывается из IRQ.
static bool blink_running;
static volatile bool blink_on;
static volatile uint32_t blink_toggles;

// Короткий обработчик прерывания. Не печатает, не ждёт и не выделяет память.
static bool on_blink(repeating_timer_t *timer)
{
    (void)timer;

    blink_on = !blink_on;
    gpio_put(LED_PIN, blink_on);
    blink_toggles++;

    // true просит SDK оставить повторяющийся будильник активным.
    return true;
}

bool platform_blink_start(uint32_t period_ms)
{
    if (period_ms < 10U || period_ms > 60000U)
    {
        return false;
    }

    // Если предыдущий запуск ещё активен, сначала корректно его снимаем.
    (void)platform_blink_stop();
    blink_toggles = 0;
    blink_on = false;

    // Отрицательный период означает отсчёт от начала прошлого callback:
    // интервалы между началами вызовов остаются привязанными к сетке времени.
    blink_running = add_repeating_timer_ms(-(int32_t)period_ms, on_blink, NULL, &blink_timer);

    return blink_running;
}

uint32_t platform_blink_stop(void)
{
    if (blink_running)
    {
        (void)cancel_repeating_timer(&blink_timer);
        blink_running = false;
    }

    blink_on = false;
    gpio_put(LED_PIN, 0);
    return blink_toggles;
}




// АЦП самостоятельно выполняет преобразования с частотой 44 100 Гц.
// Каждый готовый результат помещается в FIFO и запрашивает перенос DMA.
static void adc_free_running(void)
{
    adc_select_input(ADC_CHANNEL);
    adc_fifo_setup(true, true, 1, false, false);
    adc_set_clkdiv(ADC_CLOCK_HZ / PLATFORM_SAMPLE_RATE_HZ - 1);
}

bool platform_block_start(uint32_t count)
{
    if (adc_user != ADC_FREE || count == 0U || count > PLATFORM_BLOCK_MAX)
    {
        return false;
    }

    adc_free_running();

    // DMA читает всегда один адрес FIFO и записывает в следующий элемент массива.
    dma_channel_config cfg = dma_channel_get_default_config(block_channel);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);

    block_ready = false;
    adc_user = ADC_BLOCK;

    // Настраиваем и запускаем DMA. Он ждёт DREQ от FIFO АЦП.
    dma_channel_configure(block_channel, &cfg, block_samples, &adc_hw->fifo, count, true);
    block_start_us = time_us_32();
    adc_run(true);

    return true;
}

bool platform_block_done(void)
{
    return block_ready;
}

const uint16_t *platform_block_samples(void)
{
    return block_samples;
}

uint32_t platform_block_time_us(void)
{
    return block_end_us - block_start_us;
}

// Состояние одного конечного захвата. Массивы статические:
// обработчик не выделяет память и не кладёт большой буфер на стек.
static repeating_timer_t capture_timer;
static uint16_t capture_samples[PLATFORM_CAPTURE_MAX];
static uint32_t capture_times[PLATFORM_CAPTURE_MAX];
static uint32_t capture_count;
static uint32_t capture_period_us;
static volatile uint32_t capture_index;
static volatile uint32_t capture_handler_us;

bool platform_adc_busy(void)
{
    return adc_user != ADC_FREE;
}

// Обработчик вызывается из контекста прерывания пула будильников SDK.
// Здесь нельзя печатать, ждать или выделять память.
static bool on_capture(repeating_timer_t *timer)
{
    (void)timer;

    const uint32_t start_us = time_us_32();
    const uint32_t index = capture_index;

    capture_times[index] = start_us;
    capture_samples[index] = adc_read();
    capture_index++;
    capture_handler_us += time_us_32() - start_us;

    if (capture_index < capture_count)
    {
        return true;
    }

    // Все отсчёты набраны: освобождаем АЦП и снимаем повторяющийся будильник.
    adc_user = ADC_FREE;
    return false;
}

bool platform_capture_start(uint32_t count, uint32_t period_us)
{
    if (adc_user != ADC_FREE || count == 0U
        || count > PLATFORM_CAPTURE_MAX || period_us == 0U)
    {
        return false;
    }

    // Вся серия снимается с GP26 / ADC0. Другие команды АЦП
    // блокируются на уровне API, пока adc_user == ADC_CAPTURE.
    adc_select_input(ADC_CHANNEL);
    capture_count = count;
    capture_period_us = period_us;
    capture_index = 0U;
    capture_handler_us = 0U;
    adc_user = ADC_CAPTURE;

    if (!add_repeating_timer_us(-(int64_t)period_us, on_capture, NULL, &capture_timer))
    {
        adc_user = ADC_FREE;
        return false;
    }

    return true;
}

bool platform_capture_done(void)
{
    return capture_count != 0U && capture_index >= capture_count;
}

void platform_capture_get(platform_capture_t *capture)
{
    if (capture == NULL)
    {
        return;
    }

    capture->samples = capture_samples;
    capture->count = capture_count;
    capture->period_us = capture_period_us;
    capture->min_interval_us = 0U;
    capture->max_interval_us = 0U;
    capture->handler_percent = 0.0f;

    // У одного отсчёта нет интервалов между соседними временами.
    if (capture_count <= 1U)
    {
        return;
    }

    uint32_t min_interval = UINT32_MAX;
    uint32_t max_interval = 0U;

    for (uint32_t i = 1U; i < capture_count; ++i)
    {
        // Беззнаковое вычитание корректно и при переходе time_us_32() через 0.
        const uint32_t interval = capture_times[i] - capture_times[i - 1U];
        if (interval < min_interval)
        {
            min_interval = interval;
        }
        if (interval > max_interval)
        {
            max_interval = interval;
        }
    }

    capture->min_interval_us = min_interval;
    capture->max_interval_us = max_interval;

    const uint32_t elapsed_us = capture_times[capture_count - 1U] - capture_times[0];
    const uint32_t handler_us = capture_handler_us;
    if (elapsed_us != 0U)
    {
        capture->handler_percent = 100.0f * (float)handler_us / (float)elapsed_us;
    }
}

bool platform_stream_start(uint8_t bits)
{
    // Дополнительная разрядность 8 бит появится в п3.2.7д.
    if (adc_user != ADC_FREE || bits != 16U)
    {
        return false;
    }

    stream_samples = PLATFORM_STREAM_BYTES / 2U;
    stream_next = 0U;
    stream_overruns = 0U;

    adc_free_running();

    // Каждый канал пишет в свой буфер, а по окончании запускает соседний.
    for (int i = 0; i < 2; ++i)
    {
        const uint next_channel = (uint)stream_channels[(i + 1) % 2];
        const uint channel = (uint)stream_channels[i];

        dma_channel_config cfg = dma_channel_get_default_config(channel);
        channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
        channel_config_set_read_increment(&cfg, false);
        channel_config_set_write_increment(&cfg, true);
        channel_config_set_dreq(&cfg, DREQ_ADC);
        channel_config_set_chain_to(&cfg, next_channel);

        dma_channel_configure(channel,
                              &cfg,
                              stream_buffers[i],
                              &adc_hw->fifo,
                              stream_samples,
                              false);
        dma_channel_acknowledge_irq0(channel);
        dma_channel_set_irq0_enabled(channel, true);
        stream_ready[i] = false;
    }

    adc_user = ADC_STREAM;
    // Канал 0 первым начинает ждать DREQ от FIFO; затем включаем конвейер АЦП.
    dma_channel_start((uint)stream_channels[0]);
    adc_run(true);
    return true;
}

void platform_stream_stop(void)
{
    if (adc_user != ADC_STREAM)
    {
        return;
    }

    // Сначала прекращаем новые преобразования АЦП.
    adc_run(false);

    // Важно разорвать CHAIN_TO до abort. Из-за RP2040-E13 передача,
    // уже находившаяся в полёте, может завершиться и запустить соседний канал.
    // Самозацепление не даёт остановке одного канала повторно взвести другой.
    for (int i = 0; i < 2; ++i)
    {
        const uint channel = (uint)stream_channels[i];
        dma_channel_config cfg = dma_get_channel_config(channel);
        channel_config_set_chain_to(&cfg, channel);
        dma_channel_set_config(channel, &cfg, false);
    }

    for (int i = 0; i < 2; ++i)
    {
        const uint channel = (uint)stream_channels[i];
        dma_channel_set_irq0_enabled(channel, false);
        dma_channel_abort(channel);
        dma_channel_acknowledge_irq0(channel);
    }

    adc_one_shot();
    adc_user = ADC_FREE;
}

const uint8_t *platform_stream_next(uint32_t *samples)
{
    if (samples == NULL)
    {
        return NULL;
    }

    *samples = 0U;
    if (!stream_ready[stream_next])
    {
        return NULL;
    }

    *samples = stream_samples;
    return stream_buffers[stream_next];
}

void platform_stream_release(void)
{
    stream_ready[stream_next] = false;
    stream_next = (stream_next + 1U) % 2U;
}

uint32_t platform_stream_overruns(void)
{
    return stream_overruns;
}

void platform_write(const void *data, uint32_t bytes)
{
    if (data == NULL || bytes == 0U)
    {
        return;
    }

    // Оба флага false: не добавлять перевод строки и не заменять LF на CRLF.
    stdio_put_string((const char *)data, (int)bytes, false, false);
}
