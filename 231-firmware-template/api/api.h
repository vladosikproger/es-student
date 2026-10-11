#pragma once

// Подключаем контракт протокола, так как API должен знать структуру command_t
#include "stdio-text-protocol.h"

// Единственная публичная функция модуля API.
// Принимает указатель (const command_t *), чтобы не копировать структуру в стеке.
void api_handle(const command_t *command);