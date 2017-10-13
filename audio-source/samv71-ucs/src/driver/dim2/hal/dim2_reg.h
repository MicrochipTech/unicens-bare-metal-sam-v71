/*
 * dim2_reg.h - Definitions for registers of DIM2
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

#ifndef DIM2_OS62420_H
#define	DIM2_OS62420_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

struct dim2_regs {
	/* 0x00 */ uint32_t MLBC0;
	/* 0x01 */ uint32_t rsvd0[1];
	/* 0x02 */ uint32_t MLBPC0;
	/* 0x03 */ uint32_t MS0;
	/* 0x04 */ uint32_t rsvd1[1];
	/* 0x05 */ uint32_t MS1;
	/* 0x06 */ uint32_t rsvd2[2];
	/* 0x08 */ uint32_t MSS;
	/* 0x09 */ uint32_t MSD;
	/* 0x0A */ uint32_t rsvd3[1];
	/* 0x0B */ uint32_t MIEN;
	/* 0x0C */ uint32_t rsvd4[1];
	/* 0x0D */ uint32_t MLBPC2;
	/* 0x0E */ uint32_t MLBPC1;
	/* 0x0F */ uint32_t MLBC1;
	/* 0x10 */ uint32_t rsvd5[0x10];
	/* 0x20 */ uint32_t HCTL;
	/* 0x21 */ uint32_t rsvd6[1];
	/* 0x22 */ uint32_t HCMR0;
	/* 0x23 */ uint32_t HCMR1;
	/* 0x24 */ uint32_t HCER0;
	/* 0x25 */ uint32_t HCER1;
	/* 0x26 */ uint32_t HCBR0;
	/* 0x27 */ uint32_t HCBR1;
	/* 0x28 */ uint32_t rsvd7[8];
	/* 0x30 */ uint32_t MDAT0;
	/* 0x31 */ uint32_t MDAT1;
	/* 0x32 */ uint32_t MDAT2;
	/* 0x33 */ uint32_t MDAT3;
	/* 0x34 */ uint32_t MDWE0;
	/* 0x35 */ uint32_t MDWE1;
	/* 0x36 */ uint32_t MDWE2;
	/* 0x37 */ uint32_t MDWE3;
	/* 0x38 */ uint32_t MCTL;
	/* 0x39 */ uint32_t MADR;
	/* 0x3A */ uint32_t rsvd8[0xB6];
	/* 0xF0 */ uint32_t ACTL;
	/* 0xF1 */ uint32_t rsvd9[3];
	/* 0xF4 */ uint32_t ACSR0;
	/* 0xF5 */ uint32_t ACSR1;
	/* 0xF6 */ uint32_t ACMR0;
	/* 0xF7 */ uint32_t ACMR1;
};

#define DIM2_MASK(n)  (~((~(uint32_t)0) << (n)))

enum {
	MLBC0_MLBLK_BIT = 7,

	MLBC0_MLBPEN_BIT = 5,

	MLBC0_MLBCLK_SHIFT = 2,
	MLBC0_MLBCLK_VAL_256FS = 0,
	MLBC0_MLBCLK_VAL_512FS = 1,
	MLBC0_MLBCLK_VAL_1024FS = 2,
	MLBC0_MLBCLK_VAL_2048FS = 3,

	MLBC0_FCNT_SHIFT = 15,
	MLBC0_FCNT_MASK = 7,
	MLBC0_FCNT_MAX_VAL = 6,

	MLBC0_MLBEN_BIT = 0,

	MIEN_CTX_BREAK_BIT = 29,
	MIEN_CTX_PE_BIT = 28,
	MIEN_CTX_DONE_BIT = 27,

	MIEN_CRX_BREAK_BIT = 26,
	MIEN_CRX_PE_BIT = 25,
	MIEN_CRX_DONE_BIT = 24,

	MIEN_ATX_BREAK_BIT = 22,
	MIEN_ATX_PE_BIT = 21,
	MIEN_ATX_DONE_BIT = 20,

	MIEN_ARX_BREAK_BIT = 19,
	MIEN_ARX_PE_BIT = 18,
	MIEN_ARX_DONE_BIT = 17,

	MIEN_SYNC_PE_BIT = 16,

	MIEN_ISOC_BUFO_BIT = 1,
	MIEN_ISOC_PE_BIT = 0,

	MLBC1_NDA_SHIFT = 8,
	MLBC1_NDA_MASK = 0xFF,

	MLBC1_CLKMERR_BIT = 7,
	MLBC1_LOCKERR_BIT = 6,

	ACTL_DMA_MODE_BIT = 2,
	ACTL_DMA_MODE_VAL_DMA_MODE_0 = 0,
	ACTL_DMA_MODE_VAL_DMA_MODE_1 = 1,
	ACTL_SCE_BIT = 0,

	HCTL_EN_BIT = 15
};

enum {
	CDT0_RPC_SHIFT = 16 + 11,
	CDT0_RPC_MASK = DIM2_MASK(5),

	CDT1_BS_ISOC_SHIFT = 0,
	CDT1_BS_ISOC_MASK = DIM2_MASK(9),

	CDT3_BD_SHIFT = 0,
	CDT3_BD_MASK = DIM2_MASK(12),
	CDT3_BD_ISOC_MASK = DIM2_MASK(13),
	CDT3_BA_SHIFT = 16,

	ADT0_CE_BIT = 15,
	ADT0_LE_BIT = 14,
	ADT0_PG_BIT = 13,

	ADT1_RDY_BIT = 15,
	ADT1_DNE_BIT = 14,
	ADT1_ERR_BIT = 13,
	ADT1_PS_BIT = 12,
	ADT1_MEP_BIT = 11,
	ADT1_BD_SHIFT = 0,
	ADT1_CTRL_ASYNC_BD_MASK = DIM2_MASK(11),
	ADT1_ISOC_SYNC_BD_MASK = DIM2_MASK(13),

	CAT_MFE_BIT = 14,

	CAT_MT_BIT = 13,

	CAT_RNW_BIT = 12,

	CAT_CE_BIT = 11,

	CAT_CT_SHIFT = 8,
	CAT_CT_VAL_SYNC = 0,
	CAT_CT_VAL_CONTROL = 1,
	CAT_CT_VAL_ASYNC = 2,
	CAT_CT_VAL_ISOC = 3,

	CAT_CL_SHIFT = 0,
	CAT_CL_MASK = DIM2_MASK(6)
};

#ifdef	__cplusplus
}
#endif

#endif	/* DIM2_OS62420_H */
