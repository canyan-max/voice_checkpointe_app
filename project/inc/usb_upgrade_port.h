/**
 **************************************************************************************************
 *@file               :   usb_upgrade_port.h
 *@brief              :   Connect USB Host and FatFs to the firmware upgrade App.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 **************************************************************************************************
 */

/* define to prevent recursive inclusion --------------------------------------------------------*/
#ifndef USB_UPGRADE_PORT_H
#define USB_UPGRADE_PORT_H

/* includes -------------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // end of __cplusplus
/* define ---------------------------------------------------------------------------------------*/

/* typedef --------------------------------------------------------------------------------------*/

/* variables ------------------------------------------------------------------------------------*/

/* function  ------------------------------------------------------------------------------------*/

/* Call from the same task that polls USB Host. */
void    usb_upgrade_port_poll(void);
void    usb_upgrade_port_request(void);
uint8_t usb_upgrade_port_is_active(void);

#ifdef __cplusplus
}
#endif // end of __cplusplus

#endif /* USB_UPGRADE_PORT_H */
/* end of file ----------------------------------------------------------------------------------*/
