/*
 * dim2_hal.h - DIM2 HAL interface
 * (MediaLB, Device Interface Macro IP, OS62420)
 *
 * Copyright (C) 2015, Microchip Technology Germany II GmbH & Co. KG
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This file is licensed under GPLv2.
 */

#ifndef _DIM2_HAL_H
#define _DIM2_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "dim2_reg.h"
#include "dim2_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The values below are specified in the hardware specification.
 * So, they should not be changed until the hardware specification changes.
 */
enum mlb_clk_speed {
	CLK_256FS = 0,
	CLK_512FS = 1,
	CLK_1024FS = 2,
	CLK_2048FS = 3,
	CLK_3072FS = 4,
	CLK_4096FS = 5,
	CLK_6144FS = 6,
	CLK_8192FS = 7,
};

struct dim_ch_state_t {
	bool ready; /* Shows readiness to enqueue next buffer */
	uint16_t done_buffers; /* Number of completed buffers */
};

struct int_ch_state {
	/* changed only in interrupt context */
	volatile int request_counter;

	/* changed only in task context */
	volatile int service_counter;

	uint8_t idx1;
	uint8_t idx2;
	uint8_t level; /* [0..2], buffering level */
};

struct dim_channel {
	struct int_ch_state state;
	uint8_t addr;
	uint16_t dbr_addr;
	uint16_t dbr_size;
	uint16_t packet_length; /*< Isochronous packet length in bytes. */
	uint16_t bytes_per_frame; /*< Synchronous bytes per frame. */
	uint16_t done_sw_buffers_number; /*< Done software buffers number. */
};

uint8_t dim_startup(struct dim2_regs *dim_base_address, uint32_t mlb_clock, uint32_t fcnt);

void dim_shutdown(void);

bool dim_get_lock_state(void);

uint16_t dim_norm_ctrl_async_buffer_size(uint16_t buf_size);

uint16_t dim_norm_isoc_buffer_size(uint16_t buf_size, uint16_t packet_length);

uint16_t dim_norm_sync_buffer_size(uint16_t buf_size, uint16_t bytes_per_frame);

uint8_t dim_init_control(struct dim_channel *ch, uint8_t is_tx, uint16_t ch_address,
		    uint16_t max_buffer_size);

uint8_t dim_init_async(struct dim_channel *ch, uint8_t is_tx, uint16_t ch_address,
		  uint16_t max_buffer_size);

uint8_t dim_init_isoc(struct dim_channel *ch, uint8_t is_tx, uint16_t ch_address,
		 uint16_t packet_length);

uint8_t dim_init_sync(struct dim_channel *ch, uint8_t is_tx, uint16_t ch_address,
		 uint16_t bytes_per_frame);

uint8_t dim_destroy_channel(struct dim_channel *ch);

void dim_service_mlb_int_irq(void);

void dim_service_ahb_int_irq(struct dim_channel *const *channels);

uint8_t dim_service_channel(struct dim_channel *ch);

struct dim_ch_state_t *dim_get_channel_state(struct dim_channel *ch,
					     struct dim_ch_state_t *state_ptr);

uint16_t dim_dbr_space(struct dim_channel *ch);

bool dim_enqueue_buffer(struct dim_channel *ch, uint32_t buffer_addr,
			uint16_t buffer_size);

bool dim_detach_buffers(struct dim_channel *ch, uint16_t buffers_number);

uint32_t dimcb_io_read(uint32_t *ptr32);

void dimcb_io_write(uint32_t *ptr32, uint32_t value);

void dimcb_on_error(uint8_t error_id, const char *error_message);

#ifdef __cplusplus
}
#endif

#endif /* _DIM2_HAL_H */
