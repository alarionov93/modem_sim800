/*
 * @brief FLASH Memory Controller (FMC) registers and control functions
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#ifndef __FMC_1125_H_
#define __FMC_1125_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup FMC_11XX CHIP: LPC11xx FLASH Memory Controller driver
 * @ingroup CHIP_1125_Drivers
 * @{
 */

/**
 * @brief FLASH Memory Controller Unit register block structure
 */
typedef struct {/*!< FMC Structure */
	__I  uint32_t  RESERVED1[4];
	__IO uint32_t  FLASHCFG;
	__I  uint32_t  RESERVED2[3];
	__IO uint32_t  FMSSTART;
	__IO uint32_t  FMSSTOP;
	__I  uint32_t  RESERVED3;
	__I  uint32_t  FMSW[4];
	__I  uint32_t  RESERVED4[25];
	__I  uint32_t  RESERVED5[976];
	__I  uint32_t  FMSTAT;
	__I  uint32_t  RESERVED6;
	__O  uint32_t  FMSTATCLR;
} LPC_FMC_T;

/**
 * @brief FLASH Access time definitions
 */
typedef enum {
	FLASHTIM_20MHZ_CPU = 0,		/*!< Flash accesses use 1 CPU clocks. Use for up to 20 MHz CPU clock*/
	FLASHTIM_40MHZ_CPU = 1,		/*!< Flash accesses use 2 CPU clocks. Use for up to 40 MHz CPU clock*/
	FLASHTIM_50MHZ_CPU = 2,		/*!< Flash accesses use 3 CPU clocks. Use for up to 50 MHz CPU clock*/
} FMC_FLASHTIM_T;

/**
 * @brief	Set FLASH access time in clocks
 * @param	clks	: Clock cycles for FLASH access (minus 1)
 * @return	Nothing
 * @note	For CPU speed up to 20MHz, use a value of 0. For up to 40MHz, use
 *			a value of 1. For up to 50MHz, use a value of 2.
 */
STATIC INLINE void Chip_FMC_SetFLASHAccess(FMC_FLASHTIM_T clks)
{
	uint32_t tmp = LPC_FMC->FLASHCFG & (~(0x3));

	/* Don't alter upper bits */
	LPC_FMC->FLASHCFG = tmp | clks;
}

/* Flash signature start and busy status bit */
#define FMC_FLASHSIG_BUSY		(1UL << 17)

/* Flash signature clear status bit */
#define FMC_FLASHSIG_STAT		(1 << 2)

/**
 * @brief	Start computation of a signature for a FLASH memory range
 * @param	start	: Starting FLASH address for computation, must be aligned on 16 byte boundary
 * @param	stop	: Ending FLASH address for computation, must be aligned on 16 byte boundary
 * @return	Nothing
 * @note	Only bits 20..4 are used for the FLASH signature computation.
 *			Use the Chip_FMC_IsSignatureBusy() function to determine when the
 *			signature computation operation is complete and use the
 *			Chip_FMC_GetSignature() function to get the computed signature.
 */
STATIC INLINE void Chip_FMC_ComputeSignature(uint32_t start, uint32_t stop)
{
	LPC_FMC->FMSSTART = (start >> 4);
	LPC_FMC->FMSTATCLR = FMC_FLASHSIG_STAT;
	LPC_FMC->FMSSTOP = (stop >> 4) | FMC_FLASHSIG_BUSY;
}

/**
 * @brief	Start computation of a signature for a FLASH memory address and block count
 * @param	start	: Starting FLASH address for computation, must be aligned on 16 byte boundary
 * @param	blocks	: Number of 16 byte blocks used for computation
 * @return	Nothing
 * @note	Only bits 20..4 are used for the FLASH signature computation.
 *			Use the Chip_FMC_IsSignatureBusy() function to determine when the
 *			signature computation operation is complete and the
 *			Chip_FMC_GetSignature() function to get the computed signature.
 */
STATIC INLINE void Chip_FMC_ComputeSignatureBlocks(uint32_t start, uint32_t blocks)
{
	Chip_FMC_ComputeSignature(start, (start + (blocks * 16)));
}

/**
 * @brief	Clear signature generation completion flag
 * @return	Nothing
 */
STATIC INLINE void Chip_FMC_ClearSignatureBusy(void)
{
	LPC_FMC->FMSTATCLR = FMC_FLASHSIG_STAT;
}

/**
 * @brief	Check for signature generation completion
 * @return	true if the signature computation is running, false if finished
 */
STATIC INLINE bool Chip_FMC_IsSignatureBusy(void)
{
	return (bool) ((LPC_FMC->FMSTAT & FMC_FLASHSIG_STAT) == 0);
}

/**
 * @brief	Returns the generated FLASH signature value
 * @param	index	: Signature index to get - use 0 to FMSW0, 1 to FMSW1, etc.
 * @return	the generated FLASH signature value
 */
STATIC INLINE uint32_t Chip_FMC_GetSignature(int index)
{
	return LPC_FMC->FMSW[index];
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __FMC_1125_H_ */
