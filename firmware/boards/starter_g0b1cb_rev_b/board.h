#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "profile.h"

typedef struct {
    uint32_t id,deadline_ms;
    uint8_t size;
    bool fd,brs,deadline_valid;
    uint8_t data[64];
} board_can_frame_t;
typedef struct {
    uint32_t rx, tx, rx_dropped, tx_dropped, tx_failed, recoveries;
    uint32_t hal_error;
    uint8_t tx_queue, rx_error, tx_error;
    bool bus_off,recovering;
} board_can_diagnostics_t;

void board_init(void);
void board_can_init(void);
void board_can_reset(uint32_t now);
osg_profile_sample_t board_read_inputs(void);
void board_module_identity(uint8_t out[16]);
uint32_t board_boot_session(void);
void board_led(bool on);
bool board_usb_present(void);
void board_uid_hex(char out[25]);
void board_fault(void);
bool board_can_receive(board_can_frame_t *frame);
bool board_can_enqueue(const board_can_frame_t *frame);
void board_can_task(uint32_t now);
board_can_diagnostics_t board_can_diagnostics(void);

#define BOARD_BOOT_CAPACITY 1984U
