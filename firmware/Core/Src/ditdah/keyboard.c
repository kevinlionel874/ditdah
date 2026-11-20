#include "ditdah/keyboard.h"

#include <stddef.h>

#include "usb_device.h"
#include "usbd_hid.h"
#include "ditdah/log.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

#define KB_QUEUE_SIZE 32u
#define KB_REPORT_SIZE 8u

typedef struct
{
  uint8_t modifier;
  uint8_t usage;
} kb_key_t;

static kb_key_t queue[KB_QUEUE_SIZE];
static uint8_t queue_head;
static uint8_t queue_tail;
static uint8_t queue_count;
static bool release_pending;
static bool was_connected;
static uint8_t report[KB_REPORT_SIZE];

bool keyboard_connected(void)
{
  return hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
}

static bool hid_idle(void)
{
  USBD_HID_HandleTypeDef *hid = (USBD_HID_HandleTypeDef *)hUsbDeviceFS.pClassData;
  return hid != NULL && hid->state == HID_IDLE;
}

static bool enqueue(uint8_t modifier, uint8_t usage)
{
  if (!keyboard_connected())
  {
    return false; /* nowhere to type; don't let keys pile up */
  }
  if (queue_count >= KB_QUEUE_SIZE)
  {
    log_printf("[usb] queue full, key dropped");
    return false;
  }
  queue[queue_head].modifier = modifier;
  queue[queue_head].usage = usage;
  queue_head = (uint8_t)((queue_head + 1u) % KB_QUEUE_SIZE);
  queue_count++;
  return true;
}

bool keyboard_type_char(char c)
{
  uint8_t modifier;
  uint8_t usage;

  if (!keyboard_map_char(c, &modifier, &usage))
  {
    log_printf("[usb] can't type 0x%02X", (unsigned)(uint8_t)c);
    return false;
  }
  return enqueue(modifier, usage);
}

bool keyboard_type_usage(uint8_t usage)
{
  return enqueue(0u, usage);
}

void keyboard_poll(void)
{
  bool connected = keyboard_connected();

  if (connected != was_connected)
  {
    was_connected = connected;
    log_printf("[usb] %s", connected ? "connected" : "disconnected");
    if (!connected)
    {
      queue_head = queue_tail = queue_count = 0u;
      release_pending = false;
    }
  }
  if (!connected || !hid_idle())
  {
    return;
  }
  for (uint8_t i = 0; i < KB_REPORT_SIZE; i++)
  {
    report[i] = 0u;
  }
  if (release_pending)
  {
    release_pending = false; /* all-zero report = every key released */
    (void)USBD_HID_SendReport(&hUsbDeviceFS, report, KB_REPORT_SIZE);
    return;
  }
  if (queue_count == 0u)
  {
    return;
  }
  report[0] = queue[queue_tail].modifier;
  report[2] = queue[queue_tail].usage;
  queue_tail = (uint8_t)((queue_tail + 1u) % KB_QUEUE_SIZE);
  queue_count--;
  if (USBD_HID_SendReport(&hUsbDeviceFS, report, KB_REPORT_SIZE) == USBD_OK)
  {
    release_pending = true;
  }
}
