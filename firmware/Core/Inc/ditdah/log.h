/* Debug log over USART1 (115200 baud, through the CP2102).
 * One call prints one line; "\r\n" is added. Keep lines short and prefix them
 * with the module name, for example: log_printf("[mprls] %s hPa", text);
 * No floating point formats (newlib-nano has no float printf). */
#ifndef DITDAH_LOG_H
#define DITDAH_LOG_H

void log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
