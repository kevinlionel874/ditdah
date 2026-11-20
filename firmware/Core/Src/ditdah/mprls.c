#include "ditdah/mprls.h"

#include "i2c.h"

#define MPRLS_ADDR (0x18u << 1)
#define MPRLS_TIMEOUT_MS 10u
#define MPRLS_FIRST_POLL_MS 6u   /* a conversion takes about 5 ms */
#define MPRLS_GIVE_UP_MS 50u

#define MPRLS_STATUS_POWERED 0x40u
#define MPRLS_STATUS_BUSY 0x20u
#define MPRLS_STATUS_INTEGRITY 0x04u
#define MPRLS_STATUS_SATURATED 0x01u

/* Transfer function A: 10% to 90% of 2^24 counts spans 0 to 25 psi. */
#define MPRLS_OUT_MIN 1677722L   /* 0x19999A */
#define MPRLS_OUT_MAX 15099494L  /* 0xE66666 */
/* 25 psi = 1723.689 hPa = 17236.89 tenths of a hPa; both sides scaled by 100. */
#define MPRLS_FULL_SCALE_DHPA_X100 1723689LL
#define MPRLS_SPAN_X100 ((MPRLS_OUT_MAX - MPRLS_OUT_MIN) * 100LL)

static uint32_t started_at;
static bool measuring;

bool mprls_present(void)
{
  return HAL_I2C_IsDeviceReady(&hi2c1, MPRLS_ADDR, 2u, MPRLS_TIMEOUT_MS) == HAL_OK;
}

bool mprls_start(uint32_t now_ms)
{
  uint8_t cmd[3] = {0xAA, 0x00, 0x00};

  measuring = HAL_I2C_Master_Transmit(&hi2c1, MPRLS_ADDR, cmd, sizeof cmd, MPRLS_TIMEOUT_MS) == HAL_OK;
  started_at = now_ms;
  return measuring;
}

mprls_status_t mprls_poll(uint32_t now_ms, int32_t *deci_hpa)
{
  uint8_t rx[4];
  uint32_t elapsed = now_ms - started_at; /* unsigned subtraction handles tick wrap-around */

  if (!measuring)
  {
    return MPRLS_FAILED;
  }
  if (elapsed < MPRLS_FIRST_POLL_MS)
  {
    return MPRLS_PENDING;
  }
  if (HAL_I2C_Master_Receive(&hi2c1, MPRLS_ADDR, rx, sizeof rx, MPRLS_TIMEOUT_MS) != HAL_OK)
  {
    measuring = false;
    return MPRLS_FAILED;
  }
  if (rx[0] & MPRLS_STATUS_BUSY)
  {
    if (elapsed > MPRLS_GIVE_UP_MS)
    {
      measuring = false;
      return MPRLS_FAILED;
    }
    return MPRLS_PENDING;
  }
  measuring = false;
  if (!(rx[0] & MPRLS_STATUS_POWERED) || (rx[0] & (MPRLS_STATUS_INTEGRITY | MPRLS_STATUS_SATURATED)))
  {
    return MPRLS_FAILED;
  }

  int32_t raw = ((int32_t)rx[1] << 16) | ((int32_t)rx[2] << 8) | (int32_t)rx[3];
  int64_t scaled = (int64_t)(raw - MPRLS_OUT_MIN) * MPRLS_FULL_SCALE_DHPA_X100;
  *deci_hpa = (int32_t)(scaled / MPRLS_SPAN_X100);
  return MPRLS_READY;
}
