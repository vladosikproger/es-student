#include "memory.h"
#include "command.h"
#include "device.h"
#include "hardware/regs/addressmap.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(void);

// ??????? ???????
extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;

#define SRAM_SIZE_BYTES (264 * 1024)
#define ROM_SIZE_BYTES  (16 * 1024)

// ???????????????? ??????????
static uint32_t data_variable = 100;   // .data
static uint32_t bss_variable;          // .bss

static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

void mem_info(void)
{
    printf("%-10s %-10s %-10s %8s\n", "area", "start", "end", "size");

    row("flash", XIP_BASE,  XIP_BASE  + PICO_FLASH_SIZE_BYTES);
    row("sram",  SRAM_BASE, SRAM_BASE + SRAM_SIZE_BYTES);
    row("rom",   ROM_BASE,  ROM_BASE  + ROM_SIZE_BYTES);

    row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);
    row("free",  (uintptr_t)&__flash_binary_end,   XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("boot2", (uintptr_t)&__boot2_start__,      (uintptr_t)&__boot2_end__);
    row("text",  (uintptr_t)&__boot2_end__,        (uintptr_t)&__etext);

    row("data flash",
        (uintptr_t)&__etext,
        (uintptr_t)&__etext + ((uintptr_t)&__data_end__ - (uintptr_t)&__data_start__));
    row("data ram", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__);

    row("bss",   (uintptr_t)&__bss_start__,  (uintptr_t)&__bss_end__);
    row("heap",  (uintptr_t)&__bss_end__,    (uintptr_t)&__HeapLimit);
    row("stack", (uintptr_t)&__StackBottom,  (uintptr_t)&__StackTop);

    uintptr_t image_size = (uintptr_t)&__flash_binary_end - (uintptr_t)&__flash_binary_start;
    uintptr_t flash_free = PICO_FLASH_SIZE_BYTES - image_size;

    uintptr_t boot2_size = (uintptr_t)&__boot2_end__ - (uintptr_t)&__boot2_start__;
    uintptr_t text_size  = (uintptr_t)&__etext      - (uintptr_t)&__boot2_end__;
    uintptr_t data_size  = (uintptr_t)&__data_end__ - (uintptr_t)&__data_start__;
    uintptr_t bss_size   = (uintptr_t)&__bss_end__  - (uintptr_t)&__bss_start__;
    uintptr_t heap_size  = (uintptr_t)&__HeapLimit  - (uintptr_t)&__bss_end__;
    uintptr_t stack_size = (uintptr_t)&__StackTop   - (uintptr_t)&__StackBottom;

    uintptr_t ram_used = data_size + bss_size;

    printf("\n");
    printf("total\n");
    printf("  flash image %8u = boot2 %u + text %u + data %u\n",
           (unsigned)image_size, (unsigned)boot2_size,
           (unsigned)text_size, (unsigned)data_size);
    printf("  flash free  %8u of %u\n",
           (unsigned)flash_free, (unsigned)PICO_FLASH_SIZE_BYTES);
    printf("  ram used    %8u = data %u + bss %u\n",
           (unsigned)ram_used, (unsigned)data_size, (unsigned)bss_size);
    printf("  ram free    %8u for heap and %u for stack\n",
           (unsigned)heap_size, (unsigned)stack_size);
}

void fw_info(void)
{
    data_variable++;
    bss_variable++;

    printf("%-16s %-11s %s\n", "object", "address", "value");

    // ????? ???????: ???????? ? Thumb-?????, ?????? ?? ??????????
    uintptr_t main_thumb = (uintptr_t)main;
    uintptr_t main_flat  = main_thumb & ~1u;
    uint16_t *main_code  = (uint16_t *)main_flat;
    printf("%-16s 0x%08x  0x%04x\n",
           "main", (unsigned)main_thumb, (unsigned)*main_code);

    uintptr_t fw_thumb = (uintptr_t)fw_info;
    uintptr_t fw_flat  = fw_thumb & ~1u;
    uint16_t *fw_code  = (uint16_t *)fw_flat;
    printf("%-16s 0x%08x  0x%04x\n",
           "fw_info", (unsigned)fw_thumb, (unsigned)*fw_code);

    // ??????? ??????
    printf("%-16s 0x%08x\n",
           "commands", (unsigned)(uintptr_t)commands);
    for (uint i = 0; i < command_count; i++)
    {
        printf("- %-12s  0x%08x\n",
               commands[i].name,
               (unsigned)(uintptr_t)commands[i].handler);
    }

    // ????????? ????????
    printf("%-16s 0x%08x  %s\n",
           "DEVICE_PROJECT", (unsigned)(uintptr_t)DEVICE_PROJECT, DEVICE_PROJECT);
    printf("%-16s 0x%08x  %s\n",
           "DEVICE_BOARD", (unsigned)(uintptr_t)DEVICE_BOARD, DEVICE_BOARD);

    // ?????????? ? ???
    printf("%-16s 0x%08x  %u\n",
           "data_variable",
           (unsigned)(uintptr_t)&data_variable,
           (unsigned)data_variable);

    printf("%-16s 0x%08x  %u\n",
           "bss_variable",
           (unsigned)(uintptr_t)&bss_variable,
           (unsigned)bss_variable);

    // ????????? ?????????? (????)
    uint32_t stack_variable = 1946;
    printf("%-16s 0x%08x  %u\n",
           "stack_variable",
           (unsigned)(uintptr_t)&stack_variable,
           (unsigned)stack_variable);

    // ???? ? ????
    uint32_t *heap_variable = malloc(sizeof(uint32_t));
    if (heap_variable != NULL)
    {
        *heap_variable = 1951;
        printf("%-16s 0x%08x  %u\n",
               "heap_variable",
               (unsigned)(uintptr_t)heap_variable,
               (unsigned)*heap_variable);
        free(heap_variable);
    }
    else
    {
        printf("%-16s malloc failed\n", "heap_variable");
    }
}
