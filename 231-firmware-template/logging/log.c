#include "log.h"
#include "device.h"

void log_version(void)
{
    // Заменили DEVICE_NAME на PROJECT_NAME
    printf("device: %s %s, built %s %s, log level %d\n",
           PROJECT_NAME, FIRMWARE_VERSION, __DATE__, __TIME__, LOG_LEVEL);
}

void log_prefix(const char *level, const char *function, int line)
{
    printf("%s %s:%d ", level, function, line);
}