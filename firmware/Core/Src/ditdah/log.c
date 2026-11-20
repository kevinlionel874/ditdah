#include "ditdah/log.h"

#include <stdarg.h>
#include <stdio.h>

#include "usart.h"

#define LOG_LINE_MAX 96u
#define LOG_TIMEOUT_MS 20u /* 96 characters take about 8.3 ms at 115200 baud */

void log_printf(const char *fmt, ...)
{
  char buf[LOG_LINE_MAX];
  va_list ap;

  va_start(ap, fmt);
  int n = vsnprintf(buf, LOG_LINE_MAX - 2u, fmt, ap);
  va_end(ap);
  if (n < 0)
  {
    return;
  }
  if (n > (int)(LOG_LINE_MAX - 3u))
  {
    n = (int)(LOG_LINE_MAX - 3u); /* line was cut short */
  }
  buf[n++] = '\r';
  buf[n++] = '\n';
  (void)HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)n, LOG_TIMEOUT_MS);
}
