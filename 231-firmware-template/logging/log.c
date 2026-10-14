#include "log.h"
#include <stdio.h> // Требуется для printf

// Функцию log_version() мы полностью удалили. Журнал больше не привязан к железу.

void log_prefix(const char *level, const char *function, int line)
{
    printf("%s %s:%d ", level, function, line);
}