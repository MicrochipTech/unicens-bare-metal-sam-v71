/*
 * dim2_hal.c - DIM2 HAL implementation
 * (MediaLB, Device Interface Macro IP, OS62420)
 *
 * Copyright (C) 2015-2016, Microchip Technology Germany II GmbH & Co. KG
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This file is licensed under GPLv2.
 */

/* Author: Andrey Shvetsov <andrey.shvetsov@k2l.de> */

#include "dim2_hal.h"
#include "dim2_errors.h"
#include "dim2_reg.h"
#include <stddef.h>

/*
 * Size factor for isochronous DBR buffer.
 * Minimal value is 3.
 */
#define ISOC_DBR_FACTOR 3u

/*
 * Number of 32-bit units for DBR map.
 *
 * 1: block size is 512, max allocation is 16K
 * 2: block size is 256, max allocation is 8K
 * 4: block size is 128, max allocation is 4K
 * 8: block size is 64, max allocation is 2K
 *
 * Min allocated space is block size.
 * Max possible allocated space is 32 blocks.
 */
#define DBR_MAP_SIZE 2

/* -------------------------------------------------------------------------- */
/* not configurable area */

#define CDT 0x00
#define ADT 0x40
#define MLB_CAT 0x80
#define AHB_CAT 0x88

#define DBR_SIZE  (16 * 1024) /* specified by IP */
#define DBR_BLOCK_SIZE  (DBR_SIZE / 32 / DBR_MAP_SIZE)

#define ROUND_UP_TO(x, d)  (((x) + (d) - 1) / (d) * (d))

/* -------------------------------------------------------------------------- */
/* generic helper functions and macros */

static inline uint32_t bit_mask(uint8_t position)
{
	return (uint32_t)1 << position;
}

static inline bool dim_on_error(uint8_t error_id, const char *error_message)
{
	dimcb_on_error(error_id, error_message);
	return false;
}

/* -------------------------------------------------------------------------- */
/* types and local variables */

struct async_tx_dbr {
	uint8_t ch_addr;
	uint16_t rpc;
	uint16_t wpc;
	uint16_t rest_size;
	uint16_t sz_queue[CDT0_RPC_MASK + 1];
};

struct lld_global_vars_t {
	bool dim_is_initialized;
	bool mcm_is_initialized;
	struct dim2_regs *dim2; /* DIM2 core base address */
	struct async_tx_dbr atx_dbr;
	uint32_t fcnt;
	uint32_t dbr_map[DBR_MAP_SIZE];
};

static struct lld_global_vars_t g = { false };

/* -------------------------------------------------------------------------- */

static int dbr_get_mask_size(uint16_t size)
{
	int i;

	for (i = 0; i < 6; i++)
		if (size <= (DBR_BLOCK_SIZE << i))
			return 1 << i;
	return 0;
}

/**
 * Allocates DBR memory.
 * @param size Allocating memory size.
 * @return Offset in DBR memory by success or DBR_SIZE if out of memory.
 */
static int alloc_dbr(uint16_t size)
{
	int mask_size;
	int i, block_idx = 0;

	if (size <= 0)
		return DBR_SIZE; /* out of memory */

	mask_size = dbr_get_mask_size(size);
	if (mask_size == 0)
		return DBR_SIZE; /* out of memory */

	for (i = 0; i < DBR_MAP_SIZE; i++) {
		uint32_t const blocks = (size + DBR_BLOCK_SIZE - 1) / DBR_BLOCK_SIZE;
		uint32_t mask = ~((~(uint32_t)0) << blocks);

		do {
			if ((g.dbr_map[i] & mask) == 0) {
				g.dbr_map[i] |= mask;
				return block_idx * DBR_BLOCK_SIZE;
			}
			block_idx += mask_size;
			/* do shift left with 2 steps in case mask_size == 32 */
			mask <<= mask_size - 1;
		} while ((mask <<= 1) != 0);
	}

	return DBR_SIZE; /* out of memory */
}

static void free_dbr(int offs, int size)
{
	int block_idx = offs / DBR_BLOCK_SIZE;
	uint32_t const blocks = (size + DBR_BLOCK_SIZE - 1) / DBR_BLOCK_SIZE;
	uint32_t mask = ~((~(uint32_t)0) << blocks);

	mask <<= block_idx % 32;
	g.dbr_map[block_idx / 32] &= ~mask;
}

/* -------------------------------------------------------------------------- */

static void dim2_transfer_madr(uint32_t val)
{
	dimcb_io_write(&g.dim2->MADR, val);

	/* wait for transfer completion */
	while ((dimcb_io_read(&g.dim2->MCTL) & 1) != 1)
		continue;

	dimcb_io_write(&g.dim2->MCTL, 0);   /* clear transfer complete */
}

static void dim2_clear_dbr(uint16_t addr, uint16_t size)
{
	enum { MADR_TB_BIT = 30, MADR_WNR_BIT = 31 };

	uint16_t const end_addr = addr + size;
	uint32_t const cmd = bit_mask(MADR_WNR_BIT) | bit_mask(MADR_TB_BIT);

	dimcb_io_write(&g.dim2->MCTL, 0);   /* clear transfer complete */
	dimcb_io_write(&g.dim2->MDAT0, 0);

	for (; addr < end_addr; addr++)
		dim2_transfer_madr(cmd | addr);
}

static uint32_t dim2_read_ctr(uint32_t ctr_addr, uint16_t mdat_idx)
{
	dim2_transfer_madr(ctr_addr);

	return dimcb_io_read((&g.dim2->MDAT0) + mdat_idx);
}

static void dim2_write_ctr_mask(uint32_t ctr_addr, const uint32_t *mask, const uint32_t *value)
{
	enum { MADR_WNR_BIT = 31 };

	dimcb_io_write(&g.dim2->MCTL, 0);   /* clear transfer complete */

	if (mask[0] != 0)
		dimcb_io_write(&g.dim2->MDAT0, value[0]);
	if (mask[1] != 0)
		dimcb_io_write(&g.dim2->MDAT1, value[1]);
	if (mask[2] != 0)
		dimcb_io_write(&g.dim2->MDAT2, value[2]);
	if (mask[3] != 0)
		dimcb_io_write(&g.dim2->MDAT3, value[3]);

	dimcb_io_write(&g.dim2->MDWE0, mask[0]);
	dimcb_io_write(&g.dim2->MDWE1, mask[1]);
	dimcb_io_write(&g.dim2->MDWE2, mask[2]);
	dimcb_io_write(&g.dim2->MDWE3, mask[3]);

	dim2_transfer_madr(bit_mask(MADR_WNR_BIT) | ctr_addr);
}

static inline void dim2_write_ctr(uint32_t ctr_addr, const uint32_t *value)
{
	uint32_t const mask[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

	dim2_write_ctr_mask(ctr_addr, mask, value);
}

static inline void dim2_clear_ctr(uint32_t ctr_addr)
{
	uint32_t const value[4] = { 0, 0, 0, 0 };

	dim2_write_ctr(ctr_addr, value);
}

static void dim2_configure_cat(uint8_t cat_base, uint8_t ch_addr, uint8_t ch_type,
			       bool read_not_write, bool sync_mfe)
{
	uint16_t const cat =
		(read_not_write << CAT_RNW_BIT) |
		(ch_type << CAT_CT_SHIFT) |
		(ch_addr << CAT_CL_SHIFT) |
		(sync_mfe << CAT_MFE_BIT) |
		(false << CAT_MT_BIT) |
		(true << CAT_CE_BIT);
	uint8_t const ctr_addr = cat_base + ch_addr / 8;
	uint8_t const idx = (ch_addr % 8) / 2;
	uint8_t const shift = (ch_addr % 2) * 16;
	uint32_t mask[4] = { 0, 0, 0, 0 };
	uint32_t value[4] = { 0, 0, 0, 0 };

	mask[idx] = (uint32_t)0xFFFF << shift;
	value[idx] = cat << shift;
	dim2_write_ctr_mask(ctr_addr, mask, value);
}

static void dim2_clear_cat(uint8_t cat_base, uint8_t ch_addr)
{
	uint8_t const ctr_addr = cat_base + ch_addr / 8;
	uint8_t const idx = (ch_addr % 8) / 2;
	uint8_t const shift = (ch_addr % 2) * 16;
	uint32_t mask[4] = { 0, 0, 0, 0 };
	uint32_t value[4] = { 0, 0, 0, 0 };

	mask[idx] = (uint32_t)0xFFFF << shift;
	dim2_write_ctr_mask(ctr_addr, mask, value);
}

static void dim2_configure_cdt(uint8_t ch_addr, uint16_t dbr_address, uint16_t hw_buffer_size,
			       uint16_t packet_length)
{
	uint32_t cdt[4] = { 0, 0, 0, 0 };

	if (packet_length)
		cdt[1] = ((packet_length - 1) << CDT1_BS_ISOC_SHIFT);

	cdt[3] =
		((hw_buffer_size - 1) << CDT3_BD_SHIFT) |
		(dbr_address << CDT3_BA_SHIFT);
	dim2_write_ctr(CDT + ch_addr, cdt);
}

static uint16_t dim2_rpc(uint8_t ch_addr)
{
	uint32_t cdt0 = dim2_read_ctr(CDT + ch_addr, 0);

	return (cdt0 >> CDT0_RPC_SHIFT) & CDT0_RPC_MASK;
}

static void dim2_clear_cdt(uint8_t ch_addr)
{
	uint32_t cdt[4] = { 0, 0, 0, 0 };

	dim2_write_ctr(CDT + ch_addr, cdt);
}

static void dim2_configure_adt(uint8_t ch_addr)
{
	uint32_t adt[4] = { 0, 0, 0, 0 };

	adt[0] =
		(true << ADT0_CE_BIT) |
		(true << ADT0_LE_BIT) |
		(0 << ADT0_PG_BIT);

	dim2_write_ctr(ADT + ch_addr, adt);
}

static void dim2_clear_adt(uint8_t ch_addr)
{
	uint32_t adt[4] = { 0, 0, 0, 0 };

	dim2_write_ctr(ADT + ch_addr, adt);
}

static void dim2_start_ctrl_async(uint8_t ch_addr, uint8_t idx, uint32_t buf_addr,
				  uint16_t buffer_size)
{
	uint8_t const shift = idx * 16;

	uint32_t mask[4] = { 0, 0, 0, 0 };
	uint32_t adt[4] = { 0, 0, 0, 0 };

	mask[1] =
		bit_mask(ADT1_PS_BIT + shift) |
		bit_mask(ADT1_RDY_BIT + shift) |
		(ADT1_CTRL_ASYNC_BD_MASK << (ADT1_BD_SHIFT + shift));
	adt[1] =
		(true << (ADT1_PS_BIT + shift)) |
		(true << (ADT1_RDY_BIT + shift)) |
		((buffer_size - 1) << (ADT1_BD_SHIFT + shift));

	mask[idx + 2] = 0xFFFFFFFF;
	adt[idx + 2] = buf_addr;

	dim2_write_ctr_mask(ADT + ch_addr, mask, adt);
}

static void dim2_start_isoc_sync(uint8_t ch_addr, uint8_t idx, uint32_t buf_addr,
				 uint16_t buffer_size)
{
	uint8_t const shift = idx * 16;

	uint32_t mask[4] = { 0, 0, 0, 0 };
	uint32_t adt[4] = { 0, 0, 0, 0 };

	mask[1] =
		bit_mask(ADT1_RDY_BIT + shift) |
		(ADT1_ISOC_SYNC_BD_MASK << (ADT1_BD_SHIFT + shift));
	adt[1] =
		(true << (ADT1_RDY_BIT + shift)) |
		((buffer_size - 1) << (ADT1_BD_SHIFT + shift));

	mask[idx + 2] = 0xFFFFFFFF;
	adt[idx + 2] = buf_addr;

	dim2_write_ctr_mask(ADT + ch_addr, mask, adt);
}

static void dim2_clear_ctram(void)
{
	uint32_t ctr_addr;

	for (ctr_addr = 0; ctr_addr < 0x90; ctr_addr++)
		dim2_clear_ctr(ctr_addr);
}

static void dim2_configure_channel(
	uint8_t ch_addr, uint8_t type, uint8_t is_tx, uint16_t dbr_address, uint16_t hw_buffer_size,
	uint16_t packet_length, bool sync_mfe)
{
	dim2_configure_cdt(ch_addr, dbr_address, hw_buffer_size, packet_length);
	dim2_configure_cat(MLB_CAT, ch_addr, type, is_tx ? 1 : 0, sync_mfe);

	dim2_configure_adt(ch_addr);
	dim2_configure_cat(AHB_CAT, ch_addr, type, is_tx ? 0 : 1, sync_mfe);

	/* unmask interrupt for used channel, enable mlb_sys_int[0] interrupt */
	dimcb_io_write(&g.dim2->ACMR0,
		       dimcb_io_read(&g.dim2->ACMR0) | bit_mask(ch_addr));
}

static void dim2_clear_channel(uint8_t ch_addr)
{
	/* mask interrupt for used channel, disable mlb_sys_int[0] interrupt */
	dimcb_io_write(&g.dim2->ACMR0,
		       dimcb_io_read(&g.dim2->ACMR0) & ~bit_mask(ch_addr));

	dim2_clear_cat(AHB_CAT, ch_addr);
	dim2_clear_adt(ch_addr);

	dim2_clear_cat(MLB_CAT, ch_addr);
	dim2_clear_cdt(ch_addr);

	/* clear channel status bit */
	dimcb_io_write(&g.dim2->ACSR0, bit_mask(ch_addr));
}

/* -------------------------------------------------------------------------- */
/* trace async tx dbr fill state */

static inline uint16_t norm_pc(uint16_t pc)
{
	return pc & CDT0_RPC_MASK;
}

static void dbrcnt_init(uint8_t ch_addr, uint16_t dbr_size)
{
	g.atx_dbr.rest_size = dbr_size;
	g.atx_dbr.rpc = dim2_rpc(ch_addr);
	g.atx_dbr.wpc = g.atx_dbr.rpc;
}

static void dbrcnt_enq(int buf_sz)
{
	g.atx_dbr.rest_size -= buf_sz;
	g.atx_dbr.sz_queue[norm_pc(g.atx_dbr.wpc)] = buf_sz;
	g.atx_dbr.wpc++;
}

uint16_t dim_dbr_space(struct dim_channel *ch)
{
	uint16_t cur_rpc;
	struct async_tx_dbr *dbr = &g.atx_dbr;

	if (ch->addr != dbr->ch_addr)
		return 0xFFFF;

	cur_rpc = dim2_rpc(ch->addr);

	while (norm_pc(dbr->rpc) != cur_rpc) {
		dbr->rest_size += dbr->sz_queue[norm_pc(dbr->rpc)];
		dbr->rpc++;
	}

	if ((uint16_t)(dbr->wpc - dbr->rpc) >= CDT0_RPC_MASK)
		return 0;

	return dbr->rest_size;
}

/* -------------------------------------------------------------------------- */
/* channel state helpers */

static void state_init(struct int_ch_state *state)
{
	state->request_counter = 0;
	state->service_counter = 0;

	state->idx1 = 0;
	state->idx2 = 0;
	state->level = 0;
}

/* -------------------------------------------------------------------------- */
/* macro helper functions */

static inline bool check_channel_address(uint32_t ch_address)
{
	return ch_address > 0 && (ch_address % 2) == 0 &&
	       (ch_address / 2) <= (uint32_t)CAT_CL_MASK;
}

static inline bool check_packet_length(uint32_t packet_length)
{
	uint16_t const max_size = ((uint16_t)CDT3_BD_ISOC_MASK + 1u) / ISOC_DBR_FACTOR;

	if (packet_length <= 0)
		return false; /* too small */

	if (packet_length > max_size)
		return false; /* too big */

	if (packet_length - 1u > (uint32_t)CDT1_BS_ISOC_MASK)
		return false; /* too big */

	return true;
}

static inline bool check_bytes_per_frame(uint32_t bytes_per_frame)
{
	uint16_t const bd_factor = g.fcnt + 2;
	uint16_t const max_size = ((uint16_t)CDT3_BD_MASK + 1u) >> bd_factor;

	if (bytes_per_frame <= 0)
		return false; /* too small */

	if (bytes_per_frame > max_size)
		return false; /* too big */

	return true;
}

static inline uint16_t norm_ctrl_async_buffer_size(uint16_t buf_size)
{
	uint16_t const max_size = (uint16_t)ADT1_CTRL_ASYNC_BD_MASK + 1u;

	if (buf_size > max_size)
		return max_size;

	return buf_size;
}

static inline uint16_t norm_isoc_buffer_size(uint16_t buf_size, uint16_t packet_length)
{
	uint16_t n;
	uint16_t const max_size = (uint16_t)ADT1_ISOC_SYNC_BD_MASK + 1u;

	if (buf_size > max_size)
		buf_size = max_size;

	n = buf_size / packet_length;

	if (n < 2u)
		return 0; /* too small buffer for given packet_length */

	return packet_length * n;
}

static inline uint16_t norm_sync_buffer_size(uint16_t buf_size, uint16_t bytes_per_frame)
{
	uint16_t n;
	uint16_t const max_size = (uint16_t)ADT1_ISOC_SYNC_BD_MASK + 1u;
	uint32_t const unit = bytes_per_frame << g.fcnt;

	if (buf_size > max_size)
		buf_size = max_size;

	n = buf_size / unit;

	if (n < 1u)
		return 0; /* too small buffer for given bytes_per_frame */

	return unit * n;
}

static void dim2_cleanup(void)
{
	/* disable MediaLB */
	dimcb_io_write(&g.dim2->MLBC0, false << MLBC0_MLBEN_BIT);

	dim2_clear_ctram();

	/* disable mlb_int interrupt */
	dimcb_io_write(&g.dim2->MIEN, 0);

	/* clear status for all dma channels */
	dimcb_io_write(&g.dim2->ACSR0, 0xFFFFFFFF);
	dimcb_io_write(&g.dim2->ACSR1, 0xFFFFFFFF);

	/* mask interrupts for all channels */
	dimcb_io_write(&g.dim2->ACMR0, 0);
	dimcb_io_write(&g.dim2->ACMR1, 0);
}

static void dim2_initialize(bool enable_6pin, uint8_t mlb_clock)
{
	dim2_cleanup();

	/* configure and enable MediaLB */
	dimcb_io_write(&g.dim2->MLBC0,
		       enable_6pin << MLBC0_MLBPEN_BIT |
		       mlb_clock << MLBC0_MLBCLK_SHIFT |
		       g.fcnt << MLBC0_FCNT_SHIFT |
		       true << MLBC0_MLBEN_BIT);

	/* activate all HBI channels */
	dimcb_io_write(&g.dim2->HCMR0, 0xFFFFFFFF);
	dimcb_io_write(&g.dim2->HCMR1, 0xFFFFFFFF);

	/* enable HBI */
	dimcb_io_write(&g.dim2->HCTL, bit_mask(HCTL_EN_BIT));

	/* configure DMA */
	dimcb_io_write(&g.dim2->ACTL,
		       ACTL_DMA_MODE_VAL_DMA_MODE_1 << ACTL_DMA_MODE_BIT |
		       true << ACTL_SCE_BIT);
}

static bool dim2_is_mlb_locked(void)
{
	uint32_t const mask0 = bit_mask(MLBC0_MLBLK_BIT);
	uint32_t const mask1 = bit_mask(MLBC1_CLKMERR_BIT) |
			  bit_mask(MLBC1_LOCKERR_BIT);
	uint32_t const c1 = dimcb_io_read(&g.dim2->MLBC1);
	uint32_t const nda_mask = (uint32_t)MLBC1_NDA_MASK << MLBC1_NDA_SHIFT;

	dimcb_io_write(&g.dim2->MLBC1, c1 & nda_mask);
	return (dimcb_io_read(&g.dim2->MLBC1) & mask1) == 0 &&
	       (dimcb_io_read(&g.dim2->MLBC0) & mask0) != 0;
}

/* -------------------------------------------------------------------------- */
/* channel help routines */

static inline bool service_channel(uint8_t ch_addr, uint8_t idx)
{
	uint8_t const shift = idx * 16;
	uint32_t const adt1 = dim2_read_ctr(ADT + ch_addr, 1);
	uint32_t mask[4] = { 0, 0, 0, 0 };
	uint32_t adt_w[4] = { 0, 0, 0, 0 };

	if (((adt1 >> (ADT1_DNE_BIT + shift)) & 1) == 0)
		return false;

	mask[1] =
		bit_mask(ADT1_DNE_BIT + shift) |
		bit_mask(ADT1_ERR_BIT + shift) |
		bit_mask(ADT1_RDY_BIT + shift);
	dim2_write_ctr_mask(ADT + ch_addr, mask, adt_w);

	/* clear channel status bit */
	dimcb_io_write(&g.dim2->ACSR0, bit_mask(ch_addr));

	return true;
}

/* -------------------------------------------------------------------------- */
/* channel init routines */

static void isoc_init(struct dim_channel *ch, uint8_t ch_addr, uint16_t packet_length)
{
	state_init(&ch->state);

	ch->addr = ch_addr;

	ch->packet_length = packet_length;
	ch->bytes_per_frame = 0;
	ch->done_sw_buffers_number = 0;
}

static void sync_init(struct dim_channel *ch, uint8_t ch_addr, uint16_t bytes_per_frame)
{
	state_init(&ch->state);

	ch->addr = ch_addr;

	ch->packet_length = 0;
	ch->bytes_per_frame = bytes_per_frame;
	ch->done_sw_buffers_number = 0;
}

static void channel_init(struct dim_channel *ch, uint8_t ch_addr)
{
	state_init(&ch->state);

	ch->addr = ch_addr;

	ch->packet_length = 0;
	ch->bytes_per_frame = 0;
	ch->done_sw_buffers_number = 0;
}

/* returns true if channel interrupt state is cleared */
static bool channel_service_interrupt(struct dim_channel *ch)
{
	struct int_ch_state *const state = &ch->state;

	if (!service_channel(ch->addr, state->idx2))
		return false;

	state->idx2 ^= 1;
	state->request_counter++;
	return true;
}

static bool channel_start(struct dim_channel *ch, uint32_t buf_addr, uint16_t buf_size)
{
	struct int_ch_state *const state = &ch->state;

	if (buf_size <= 0)
		return dim_on_error(DIM_ERR_BAD_BUFFER_SIZE, "Bad buffer size");

	if (ch->packet_length == 0 && ch->bytes_per_frame == 0 &&
	    buf_size != norm_ctrl_async_buffer_size(buf_size))
		return dim_on_error(DIM_ERR_BAD_BUFFER_SIZE,
				    "Bad control/async buffer size");

	if (ch->packet_length &&
	    buf_size != norm_isoc_buffer_size(buf_size, ch->packet_length))
		return dim_on_error(DIM_ERR_BAD_BUFFER_SIZE,
				    "Bad isochronous buffer size");

	if (ch->bytes_per_frame &&
	    buf_size != norm_sync_buffer_size(buf_size, ch->bytes_per_frame))
		return dim_on_error(DIM_ERR_BAD_BUFFER_SIZE,
				    "Bad synchronous buffer size");

	if (state->level >= 2u)
		return dim_on_error(DIM_ERR_OVERFLOW, "Channel overflow");

	++state->level;

	if (ch->addr == g.atx_dbr.ch_addr)
		dbrcnt_enq(buf_size);

	if (ch->packet_length || ch->bytes_per_frame)
		dim2_start_isoc_sync(ch->addr, state->idx1, buf_addr, buf_size);
	else
		dim2_start_ctrl_async(ch->addr, state->idx1, buf_addr,
				      buf_size);
	state->idx1 ^= 1;

	return true;
}

static uint8_t channel_service(struct dim_channel *ch)
{
	struct int_ch_state *const state = &ch->state;

	if (state->service_counter != state->request_counter) {
		state->service_counter++;
		if (state->level == 0)
			return DIM_ERR_UNDERFLOW;

		--state->level;
		ch->done_sw_buffers_number++;
	}

	return DIM_NO_ERROR;
}

static bool channel_detach_buffers(struct dim_channel *ch, uint16_t buffers_number)
{
	if (buffers_number > ch->done_sw_buffers_number)
		return dim_on_error(DIM_ERR_UNDERFLOW, "Channel underflow");

	ch->done_sw_buffers_number -= buffers_number;
	return true;
}

/* -------------------------------------------------------------------------- */
/* API */

uint8_t dim_startup(struct dim2_regs *dim_base_address, uint32_t mlb_clock, uint32_t fcnt)
{
	g.dim_is_initialized = false;

	if (!dim_base_address)
		return DIM_INIT_ERR_DIM_ADDR;

	/* MediaLB clock: 0 - 256 fs, 1 - 512 fs, 2 - 1024 fs, 3 - 2048 fs */
	/* MediaLB clock: 4 - 3072 fs, 5 - 4096 fs, 6 - 6144 fs, 7 - 8192 fs */
	if (mlb_clock >= 8)
		return DIM_INIT_ERR_MLB_CLOCK;

	if (fcnt > MLBC0_FCNT_MAX_VAL)
		return DIM_INIT_ERR_MLB_CLOCK;

	g.dim2 = dim_base_address;
	g.fcnt = fcnt;
	g.dbr_map[0] = 0;
	g.dbr_map[1] = 0;

	dim2_initialize(mlb_clock >= 3, mlb_clock);

	g.dim_is_initialized = true;

	return DIM_NO_ERROR;
}

void dim_shutdown(void)
{
	g.dim_is_initialized = false;
	dim2_cleanup();
}

bool dim_get_lock_state(void)
{
	return dim2_is_mlb_locked();
}

static uint8_t init_ctrl_async(struct dim_channel *ch, uint8_t type, uint8_t is_tx,
			  uint16_t ch_address, uint16_t hw_buffer_size)
{
	if (!g.dim_is_initialized || !ch)
		return DIM_ERR_DRIVER_NOT_INITIALIZED;

	if (!check_channel_address(ch_address))
		return DIM_INIT_ERR_CHANNEL_ADDRESS;

	ch->dbr_size = ROUND_UP_TO(hw_buffer_size, DBR_BLOCK_SIZE);
	ch->dbr_addr = alloc_dbr(ch->dbr_size);
	if (ch->dbr_addr >= DBR_SIZE)
		return DIM_INIT_ERR_OUT_OF_MEMORY;

	channel_init(ch, ch_address / 2);

	dim2_configure_channel(ch->addr, type, is_tx,
			       ch->dbr_addr, ch->dbr_size, 0, false);

	return DIM_NO_ERROR;
}

void dim_service_mlb_int_irq(void)
{
	dimcb_io_write(&g.dim2->MS0, 0);
	dimcb_io_write(&g.dim2->MS1, 0);
}

uint16_t dim_norm_ctrl_async_buffer_size(uint16_t buf_size)
{
	return norm_ctrl_async_buffer_size(buf_size);
}

/**
 * Retrieves maximal possible correct buffer size for isochronous data type
 * conform to given packet length and not bigger than given buffer size.
 *
 * Returns non-zero correct buffer size or zero by error.
 */
uint16_t dim_norm_isoc_buffer_size(uint16_t buf_size, uint16_t packet_length)
{
	if (!check_packet_length(packet_length))
		return 0;

	return norm_isoc_buffer_size(buf_size, packet_length);
}

/**
 * Retrieves maximal possible correct buffer size for synchronous data type
 * conform to given bytes per frame and not bigger than given buffer size.
 *
 * Returns non-zero correct buffer size or zero by error.
 */
uint16_t dim_norm_sync_buffer_size(uint16_t buf_size, uint16_t bytes_per_frame)
{
	if (!check_bytes_per_frame(bytes_per_frame))
		return 0;

	return norm_sync_buffer_size(buf_size, bytes_per_frame);
}

uint8_t dim_init_control(struct dim_channel *ch, uint8_t is_tx, uint16_t ch_address,
		    uint16_t max_buffer_size)
{
	return init_ctrl_async(ch, CAT_CT_VAL_CONTROL, is_tx, ch_address,
			       max_buffer_size);
}

uint8_t dim_init_async(struct dim_channel *ch, uint8_t is_tx, uint16_t ch_address,
		  uint16_t max_buffer_size)
{
	uint8_t ret = init_ctrl_async(ch, CAT_CT_VAL_ASYNC, is_tx, ch_address,
				 max_buffer_size);

	if (is_tx && !g.atx_dbr.ch_addr) {
		g.atx_dbr.ch_addr = ch->addr;
		dbrcnt_init(ch->addr, ch->dbr_size);
		dimcb_io_write(&g.dim2->MIEN, bit_mask(20));
	}

	return ret;
}

uint8_t dim_init_isoc(struct dim_channel *ch, uint8_t is_tx, uint16_t ch_address,
		 uint16_t packet_length)
{
	if (!g.dim_is_initialized || !ch)
		return DIM_ERR_DRIVER_NOT_INITIALIZED;

	if (!check_channel_address(ch_address))
		return DIM_INIT_ERR_CHANNEL_ADDRESS;

	if (!check_packet_length(packet_length))
		return DIM_ERR_BAD_CONFIG;

	ch->dbr_size = packet_length * ISOC_DBR_FACTOR;
	ch->dbr_addr = alloc_dbr(ch->dbr_size);
	if (ch->dbr_addr >= DBR_SIZE)
		return DIM_INIT_ERR_OUT_OF_MEMORY;

	isoc_init(ch, ch_address / 2, packet_length);

	dim2_configure_channel(ch->addr, CAT_CT_VAL_ISOC, is_tx, ch->dbr_addr,
			       ch->dbr_size, packet_length, false);

	return DIM_NO_ERROR;
}

uint8_t dim_init_sync(struct dim_channel *ch, uint8_t is_tx, uint16_t ch_address,
		 uint16_t bytes_per_frame)
{
	uint16_t bd_factor = g.fcnt + 2;

	if (!g.dim_is_initialized || !ch)
		return DIM_ERR_DRIVER_NOT_INITIALIZED;

	if (!check_channel_address(ch_address))
		return DIM_INIT_ERR_CHANNEL_ADDRESS;

	if (!check_bytes_per_frame(bytes_per_frame))
		return DIM_ERR_BAD_CONFIG;

	ch->dbr_size = bytes_per_frame << bd_factor;
	ch->dbr_addr = alloc_dbr(ch->dbr_size);
	if (ch->dbr_addr >= DBR_SIZE)
		return DIM_INIT_ERR_OUT_OF_MEMORY;

	sync_init(ch, ch_address / 2, bytes_per_frame);

	dim2_clear_dbr(ch->dbr_addr, ch->dbr_size);
	dim2_configure_channel(ch->addr, CAT_CT_VAL_SYNC, is_tx,
			       ch->dbr_addr, ch->dbr_size, 0, true);

	return DIM_NO_ERROR;
}

uint8_t dim_destroy_channel(struct dim_channel *ch)
{
	if (!g.dim_is_initialized || !ch)
		return DIM_ERR_DRIVER_NOT_INITIALIZED;

	if (ch->addr == g.atx_dbr.ch_addr) {
		dimcb_io_write(&g.dim2->MIEN, 0);
		g.atx_dbr.ch_addr = 0;
	}

	dim2_clear_channel(ch->addr);
	if (ch->dbr_addr < DBR_SIZE)
		free_dbr(ch->dbr_addr, ch->dbr_size);
	ch->dbr_addr = DBR_SIZE;

	return DIM_NO_ERROR;
}

void dim_service_ahb_int_irq(struct dim_channel *const *channels)
{
	bool state_changed;

	if (!g.dim_is_initialized) {
		dim_on_error(DIM_ERR_DRIVER_NOT_INITIALIZED,
			     "DIM is not initialized");
		return;
	}

	if (!channels) {
		dim_on_error(DIM_ERR_DRIVER_NOT_INITIALIZED, "Bad channels");
		return;
	}

	/*
	 * Use while-loop and a flag to make sure the age is changed back at
	 * least once, otherwise the interrupt may never come if CPU generates
	 * interrupt on changing age.
	 * This cycle runs not more than number of channels, because
	 * channel_service_interrupt() routine doesn't start the channel again.
	 */
	do {
		struct dim_channel *const *ch = channels;

		state_changed = false;

		while (*ch) {
			state_changed |= channel_service_interrupt(*ch);
			++ch;
		}
	} while (state_changed);
}

uint8_t dim_service_channel(struct dim_channel *ch)
{
	if (!g.dim_is_initialized || !ch)
		return DIM_ERR_DRIVER_NOT_INITIALIZED;

	return channel_service(ch);
}

struct dim_ch_state_t *dim_get_channel_state(struct dim_channel *ch,
					     struct dim_ch_state_t *state_ptr)
{
	if (!ch || !state_ptr)
		return NULL;

	state_ptr->ready = ch->state.level < 2;
	state_ptr->done_buffers = ch->done_sw_buffers_number;

	return state_ptr;
}

bool dim_enqueue_buffer(struct dim_channel *ch, uint32_t buffer_addr,
			uint16_t buffer_size)
{
	if (!ch)
		return dim_on_error(DIM_ERR_DRIVER_NOT_INITIALIZED,
				    "Bad channel");

	return channel_start(ch, buffer_addr, buffer_size);
}

bool dim_detach_buffers(struct dim_channel *ch, uint16_t buffers_number)
{
	if (!ch)
		return dim_on_error(DIM_ERR_DRIVER_NOT_INITIALIZED,
				    "Bad channel");

	return channel_detach_buffers(ch, buffers_number);
}
