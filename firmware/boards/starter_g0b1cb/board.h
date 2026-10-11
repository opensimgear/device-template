#pragma once
#include <stdbool.h>
#include <stdint.h>
void board_init(void);
bool board_button_pressed(void);
bool board_usb_present(void);
void board_led(bool on);
void board_uid_hex(char out[25]);
bool board_can_loopback(void);
void board_fault(void);
