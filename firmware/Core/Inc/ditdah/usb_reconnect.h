/* Pulls USB D+ (PA12) low for 10 ms so the computer sees a fresh connection after a
 * reset or a flash. Call once after SystemClock_Config() and before MX_USB_DEVICE_Init(). */
#ifndef DITDAH_USB_RECONNECT_H
#define DITDAH_USB_RECONNECT_H

void usb_force_reconnect(void);

#endif
