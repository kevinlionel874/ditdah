/* Adafruit MPRLS ported pressure sensor (Honeywell MPRLS0025PA00001A, 0-25 psi)
 * on I2C1 at address 0x18. Non-blocking: start a measurement, then poll until
 * it is ready (about 5 ms). Pressure is reported in tenths of a hectopascal
 * (10132 means 1013.2 hPa) so no floating point is needed. */
#ifndef DITDAH_MPRLS_H
#define DITDAH_MPRLS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  MPRLS_PENDING, /* measurement not finished yet: poll again later */
  MPRLS_READY,   /* *deci_hpa holds a new reading */
  MPRLS_FAILED,  /* I2C error, sensor error flag, or no answer within 50 ms */
} mprls_status_t;

bool mprls_present(void);
bool mprls_start(uint32_t now_ms);
mprls_status_t mprls_poll(uint32_t now_ms, int32_t *deci_hpa);

#endif
