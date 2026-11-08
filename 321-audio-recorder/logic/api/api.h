#pragma once

#include "stdio-text-protocol/stdio-text-protocol.h"
#include "telemetry-task/telemetry-task.h"
#include "stream-task/stream-task.h"

void api_handle(const command_t *command);

// Печатает одну запись телеметрии в текстовом формате протокола.
void api_print_telemetry(const telemetry_record_t *record);

// Callback передачи готового двоичного кадра потока.
void api_send_frame(const stream_header_t *header, const uint8_t *data, uint32_t bytes);
