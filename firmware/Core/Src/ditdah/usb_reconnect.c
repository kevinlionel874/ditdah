#include "ditdah/usb_reconnect.h"

#include "main.h"

void usb_force_reconnect(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  gpio.Pin = GPIO_PIN_12;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &gpio);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
  HAL_Delay(10); /* start-up only, so a delay is allowed here */
  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12); /* the USB peripheral takes the pin back in MX_USB_DEVICE_Init */
}
