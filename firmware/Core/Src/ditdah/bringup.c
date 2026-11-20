#include "ditdah/bringup.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "ditdah/keyboard.h"
#include "ditdah/log.h"
#include "ditdah/mprls.h"
#include "ditdah/sh1106.h"

#define HEARTBEAT_MS 250u        /* LED toggles every 250 ms: 2 blinks per second */
#define PRESSURE_PERIOD_MS 100u  /* 10 readings per second */
#define BASELINE_SAMPLES 10u     /* first second of readings = the resting pressure */
#define DEBOUNCE_MS 10u
#define OLED_PERIOD_MS 250u
#define FAIL_LOG_EVERY 20u       /* don't flood the log when the sensor keeps failing */

#define OLED_ADDR 0x3Cu
#define MPRLS_ADDR 0x18u

typedef struct
{
  const char *name;
  GPIO_TypeDef *port;
  uint16_t pin;
  bool raw;         /* last reading */
  bool stable;      /* debounced: true = pressed */
  uint32_t changed_at;
} input_t;

static input_t inputs[] = {
  {"BIG", BTN_BIG_GPIO_Port, BTN_BIG_Pin, false, false, 0},
  {"JDIT", JACK_DIT_GPIO_Port, JACK_DIT_Pin, false, false, 0},
  {"JDAH", JACK_DAH_GPIO_Port, JACK_DAH_Pin, false, false, 0},
  {"DIT", BTN_DIT_GPIO_Port, BTN_DIT_Pin, false, false, 0},
  {"DAH", BTN_DAH_GPIO_Port, BTN_DAH_Pin, false, false, 0},
  {"MODE", BTN_MODE_GPIO_Port, BTN_MODE_Pin, false, false, 0},
};
#define INPUT_COUNT (sizeof inputs / sizeof inputs[0])

static bool oled_ok;
static bool mprls_ok;

static bool feedback_on;
static uint32_t last_heartbeat;

static bool pressure_measuring;
static uint32_t last_pressure_start;
static int32_t pressure_dhpa;
static bool pressure_valid;
static int32_t baseline_sum;
static uint32_t baseline_count;
static int32_t baseline_dhpa;
static uint32_t pressure_failures;

static uint32_t last_oled;
static uint8_t oled_next_line;   /* 0 = nothing pending; otherwise the next line to redraw */

/* "1013.2" or "+3.5" from tenths. */
static void format_deci(char *out, size_t size, int32_t value, bool show_plus)
{
  const char *sign = (value < 0) ? "-" : (show_plus ? "+" : "");
  uint32_t mag = (value < 0) ? (uint32_t)(-(int64_t)value) : (uint32_t)value;
  snprintf(out, size, "%s%lu.%lu", sign, (unsigned long)(mag / 10u), (unsigned long)(mag % 10u));
}

static void i2c_scan(void)
{
  uint32_t found = 0;

  log_printf("[i2c] scanning 0x08..0x77");
  for (uint16_t addr = 0x08; addr <= 0x77; addr++)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 1u, 2u) == HAL_OK)
    {
      const char *what = (addr == OLED_ADDR) ? " (OLED)" : (addr == MPRLS_ADDR) ? " (MPRLS)" : "";
      log_printf("[i2c] found 0x%02X%s", (unsigned)addr, what);
      found++;
      if (addr == OLED_ADDR)
      {
        oled_ok = true;
      }
      if (addr == MPRLS_ADDR)
      {
        mprls_ok = true;
      }
    }
  }
  log_printf("[i2c] %lu device(s). Expect 0x3C and 0x18.", (unsigned long)found);
  if (found == 0)
  {
    log_printf("[i2c] nothing found: check SCL=PB6, SDA=PB7, 3.3V and GND");
  }
}

static void set_feedback(bool on)
{
  if (on == feedback_on)
  {
    return;
  }
  feedback_on = on;
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET); /* LED lights when low */
  HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
  if (on)
  {
    (void)HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  }
  else
  {
    (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
  }
}

static bool update_inputs(uint32_t now)
{
  bool any_down = false;

  for (uint32_t i = 0; i < INPUT_COUNT; i++)
  {
    input_t *in = &inputs[i];
    bool down = HAL_GPIO_ReadPin(in->port, in->pin) == GPIO_PIN_RESET; /* active-low */

    if (down != in->raw)
    {
      in->raw = down;
      in->changed_at = now;
    }
    else if (down != in->stable && (now - in->changed_at) >= DEBOUNCE_MS)
    {
      in->stable = down;
      log_printf("[input] %s %s", in->name, down ? "down" : "up");
      if (down && i == 0u) /* the big button (jack 1) types "e" to test the USB keyboard */
      {
        log_printf("[usb] typing e: %s", keyboard_type_char('e') ? "queued" : "not connected");
      }
    }
    if (in->stable)
    {
      any_down = true;
    }
  }
  return any_down;
}

static void handle_sample(int32_t dhpa)
{
  char abs_text[16];
  char delta_text[16];

  pressure_dhpa = dhpa;
  pressure_valid = true;
  if (baseline_count < BASELINE_SAMPLES)
  {
    baseline_sum += dhpa;
    baseline_count++;
    if (baseline_count == BASELINE_SAMPLES)
    {
      baseline_dhpa = baseline_sum / (int32_t)BASELINE_SAMPLES;
      format_deci(abs_text, sizeof abs_text, baseline_dhpa, false);
      log_printf("[mprls] baseline %s hPa. Now blow and sip into the tube.", abs_text);
    }
    return;
  }
  format_deci(abs_text, sizeof abs_text, dhpa, false);
  format_deci(delta_text, sizeof delta_text, dhpa - baseline_dhpa, true);
  log_printf("[mprls] %s hPa  d=%s", abs_text, delta_text);
}

static void update_pressure(uint32_t now)
{
  if (!mprls_ok)
  {
    return;
  }
  if (!pressure_measuring)
  {
    if ((now - last_pressure_start) >= PRESSURE_PERIOD_MS)
    {
      last_pressure_start = now;
      pressure_measuring = mprls_start(now);
      if (!pressure_measuring && (pressure_failures++ % FAIL_LOG_EVERY) == 0u)
      {
        log_printf("[mprls] start failed (I2C)");
      }
    }
    return;
  }

  int32_t dhpa;
  mprls_status_t status = mprls_poll(now, &dhpa);
  if (status == MPRLS_PENDING)
  {
    return;
  }
  pressure_measuring = false;
  if (status == MPRLS_READY)
  {
    handle_sample(dhpa);
  }
  else if ((pressure_failures++ % FAIL_LOG_EVERY) == 0u)
  {
    log_printf("[mprls] read failed");
  }
}

static void draw_oled_line(uint8_t line)
{
  char text[32]; /* the OLED shows the first 21 characters */
  char a[16];
  char b[16];

  switch (line)
  {
    case 3:
      if (pressure_valid)
      {
        format_deci(a, sizeof a, pressure_dhpa, false);
        snprintf(text, sizeof text, "P  %s hPa", a);
      }
      else
      {
        snprintf(text, sizeof text, "P  %s", mprls_ok ? "waiting" : "no sensor");
      }
      break;
    case 4:
      if (baseline_count >= BASELINE_SAMPLES)
      {
        format_deci(b, sizeof b, pressure_dhpa - baseline_dhpa, true);
        snprintf(text, sizeof text, "d  %s hPa", b);
      }
      else
      {
        snprintf(text, sizeof text, "d  measuring rest");
      }
      break;
    case 6:
    {
      size_t used = (size_t)snprintf(text, sizeof text, "In:");
      bool any = false;
      for (uint32_t i = 0; i < INPUT_COUNT && used < sizeof text; i++)
      {
        if (inputs[i].stable)
        {
          used += (size_t)snprintf(text + used, sizeof text - used, " %s", inputs[i].name);
          any = true;
        }
      }
      if (!any)
      {
        snprintf(text, sizeof text, "In: none pressed");
      }
      break;
    }
    default:
      return;
  }
  if (!sh1106_line(line, text))
  {
    oled_ok = false;
    log_printf("[oled] write failed, OLED updates stopped");
  }
}

static void update_oled(uint32_t now)
{
  static const uint8_t live_lines[] = {3, 4, 6};
  static uint8_t index;

  if (!oled_ok)
  {
    return;
  }
  if (oled_next_line == 0u && (now - last_oled) >= OLED_PERIOD_MS)
  {
    last_oled = now;
    index = 0;
    oled_next_line = live_lines[0];
  }
  if (oled_next_line != 0u)
  {
    /* One line per pass keeps each pass short (a line takes about 12 ms over I2C). */
    draw_oled_line(oled_next_line);
    index++;
    oled_next_line = (index < sizeof live_lines) ? live_lines[index] : 0u;
  }
}

void bringup_init(void)
{
  log_printf("%s", "");
  log_printf("[boot] DitDah bring-up test, SYSCLK %lu Hz", (unsigned long)HAL_RCC_GetSysClockFreq());
  i2c_scan();

  if (oled_ok)
  {
    oled_ok = sh1106_init();
    log_printf("[oled] %s", oled_ok ? "ready" : "init failed");
  }
  if (oled_ok)
  {
    char status[SH1106_CHARS_PER_LINE + 1];
    snprintf(status, sizeof status, "OLED ok  MPRLS %s", mprls_ok ? "ok" : "--");
    (void)sh1106_line(0, "DitDah bring-up");
    (void)sh1106_line(1, status);
  }
  if (!mprls_ok)
  {
    log_printf("[mprls] not found at 0x18");
  }

  uint32_t now = HAL_GetTick();
  for (uint32_t i = 0; i < INPUT_COUNT; i++)
  {
    inputs[i].raw = HAL_GPIO_ReadPin(inputs[i].port, inputs[i].pin) == GPIO_PIN_RESET;
    inputs[i].stable = inputs[i].raw;
    inputs[i].changed_at = now;
    if (inputs[i].stable)
    {
      log_printf("[input] %s is already down at start (stuck or wired to GND?)", inputs[i].name);
    }
  }
  last_heartbeat = now;
  last_pressure_start = now;
  last_oled = now;
  log_printf("[boot] ready. Press any input: LED, motor and buzzer turn on.");
}

void bringup_loop(void)
{
  uint32_t now = HAL_GetTick();
  bool any_down = update_inputs(now);

  set_feedback(any_down);
  if (!any_down && (now - last_heartbeat) >= HEARTBEAT_MS)
  {
    last_heartbeat = now;
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  }
  update_pressure(now);
  update_oled(now);
  keyboard_poll();
}
