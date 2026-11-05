#pragma once

#include "stdio-text-protocol/stdio-text-protocol.h"
#include "telemetry-task/telemetry-task.h"

void api_handle(const command_t *command);

// Печатает одну запись телеметрии в текстовом формате протокола.
void api_print_telemetry(const telemetry_record_t *record);
