/*
 * Copyright (c) 2014, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL FREESCALE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*
 * WARNING! DO NOT EDIT THIS FILE DIRECTLY!
 *
 * This file was generated automatically and any changes may be lost.
 */
#ifndef __HW_RNG_REGISTERS_H__
#define __HW_RNG_REGISTERS_H__

#include "regs.h"

/*
 * MK64F12 RNG
 *
 * Random Number Generator Accelerator
 *
 * Registers defined in this header file:
 * - HW_RNG_CR - RNGA Control Register
 * - HW_RNG_SR - RNGA Status Register
 * - HW_RNG_ER - RNGA Entropy Register
 * - HW_RNG_OR - RNGA Output Register
 *
 * - hw_rng_t - Struct containing all module registers.
 */

/*! @name Module base addresses */
/*@{*/
#ifndef REGS_RNG_BASE
#define HW_RNG_INSTANCE_COUNT (1U) /*!< Number of instances of the RNG module. */
#define REGS_RNG_BASE (0x40029000U) /*!< Base address for RNG. */
#endif
/*@}*/

/*******************************************************************************
 * HW_RNG_CR - RNGA Control Register
 ******************************************************************************/

#ifndef __LANGUAGE_ASM__
/*!
 * @brief HW_RNG_CR - RNGA Control Register (RW)
 *
 * Reset value: 0x00000000U
 *
 * Controls the operation of RNGA.
 */
typedef union _hw_rng_cr
{
    uint32_t U;
    struct _hw_rng_cr_bitfields
    {
        uint32_t GO : 1;               /*!< [0] Go */
        uint32_t HA : 1;               /*!< [1] High Assurance */
        uint32_t INTM : 1;             /*!< [2] Interrupt Mask */
        uint32_t CLRI : 1;             /*!< [3] Clear Interrupt */
        uint32_t SLP : 1;              /*!< [4] Sleep */
        uint32_t RESERVED0 : 27;       /*!< [31:5]  */
    } B;
} hw_rng_cr_t;
#endif

/*!
 * @name Constants and macros for entire RNG_CR register
 */
/*@{*/
#define HW_RNG_CR_ADDR           (REGS_RNG_BASE + 0x0U)

#ifndef __LANGUAGE_ASM__
#define HW_RNG_CR                (*(__IO hw_rng_cr_t *) HW_RNG_CR_ADDR)
#define HW_RNG_CR_RD()           (HW_RNG_CR.U)
#define HW_RNG_CR_WR(v)          (HW_RNG_CR.U = (v))
#define HW_RNG_CR_SET(v)         (HW_RNG_CR_WR(HW_RNG_CR_RD() |  (v)))
#define HW_RNG_CR_CLR(v)         (HW_RNG_CR_WR(HW_RNG_CR_RD() & ~(v)))
#define HW_RNG_CR_TOG(v)         (HW_RNG_CR_WR(HW_RNG_CR_RD() ^  (v)))
#endif
/*@}*/

/*
 * Constants & macros for individual RNG_CR bitfields
 */

/*!
 * @name Register RNG_CR, field GO[0] (RW)
 *
 * Specifies whether random-data generation and loading (into OR[RANDOUT]) is
 * enabled.This field is sticky. You must reset RNGA to stop RNGA from loading
 * OR[RANDOUT] with data.
 *
 * Values:
 * - 0 - Disabled
 * - 1 - Enabled
 */
/*@{*/
#define BP_RNG_CR_GO         (0U)          /*!< Bit position for RNG_CR_GO. */
#define BM_RNG_CR_GO         (0x00000001U) /*!< Bit mask for RNG_CR_GO. */
#define BS_RNG_CR_GO         (1U)          /*!< Bit field size in bits for RNG_CR_GO. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_CR_GO field. */
#define BR_RNG_CR_GO         (BITBAND_ACCESS32(HW_RNG_CR_ADDR, BP_RNG_CR_GO))
#endif

/*! @brief Format value for bitfield RNG_CR_GO. */
#define BF_RNG_CR_GO(v)      (__REG_VALUE_TYPE((__REG_VALUE_TYPE((v), uint32_t) << BP_RNG_CR_GO), uint32_t) & BM_RNG_CR_GO)

#ifndef __LANGUAGE_ASM__
/*! @brief Set the GO field to a new value. */
#define BW_RNG_CR_GO(v)      (BITBAND_ACCESS32(HW_RNG_CR_ADDR, BP_RNG_CR_GO) = (v))
#endif
/*@}*/

/*!
 * @name Register RNG_CR, field HA[1] (RW)
 *
 * Enables notification of security violations (via SR[SECV]). A security
 * violation occurs when you read OR[RANDOUT] and SR[OREG_LVL]=0. This field is sticky.
 * After enabling notification of security violations, you must reset RNGA to
 * disable them again.
 *
 * Values:
 * - 0 - Disabled
 * - 1 - Enabled
 */
/*@{*/
#define BP_RNG_CR_HA         (1U)          /*!< Bit position for RNG_CR_HA. */
#define BM_RNG_CR_HA         (0x00000002U) /*!< Bit mask for RNG_CR_HA. */
#define BS_RNG_CR_HA         (1U)          /*!< Bit field size in bits for RNG_CR_HA. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_CR_HA field. */
#define BR_RNG_CR_HA         (BITBAND_ACCESS32(HW_RNG_CR_ADDR, BP_RNG_CR_HA))
#endif

/*! @brief Format value for bitfield RNG_CR_HA. */
#define BF_RNG_CR_HA(v)      (__REG_VALUE_TYPE((__REG_VALUE_TYPE((v), uint32_t) << BP_RNG_CR_HA), uint32_t) & BM_RNG_CR_HA)

#ifndef __LANGUAGE_ASM__
/*! @brief Set the HA field to a new value. */
#define BW_RNG_CR_HA(v)      (BITBAND_ACCESS32(HW_RNG_CR_ADDR, BP_RNG_CR_HA) = (v))
#endif
/*@}*/

/*!
 * @name Register RNG_CR, field INTM[2] (RW)
 *
 * Masks the triggering of an error interrupt to the interrupt controller when
 * an OR underflow condition occurs. An OR underflow condition occurs when you
 * read OR[RANDOUT] and SR[OREG_LVL]=0. See the Output Register (OR) description.
 *
 * Values:
 * - 0 - Not masked
 * - 1 - Masked
 */
/*@{*/
#define BP_RNG_CR_INTM       (2U)          /*!< Bit position for RNG_CR_INTM. */
#define BM_RNG_CR_INTM       (0x00000004U) /*!< Bit mask for RNG_CR_INTM. */
#define BS_RNG_CR_INTM       (1U)          /*!< Bit field size in bits for RNG_CR_INTM. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_CR_INTM field. */
#define BR_RNG_CR_INTM       (BITBAND_ACCESS32(HW_RNG_CR_ADDR, BP_RNG_CR_INTM))
#endif

/*! @brief Format value for bitfield RNG_CR_INTM. */
#define BF_RNG_CR_INTM(v)    (__REG_VALUE_TYPE((__REG_VALUE_TYPE((v), uint32_t) << BP_RNG_CR_INTM), uint32_t) & BM_RNG_CR_INTM)

#ifndef __LANGUAGE_ASM__
/*! @brief Set the INTM field to a new value. */
#define BW_RNG_CR_INTM(v)    (BITBAND_ACCESS32(HW_RNG_CR_ADDR, BP_RNG_CR_INTM) = (v))
#endif
/*@}*/

/*!
 * @name Register RNG_CR, field CLRI[3] (WORZ)
 *
 * Clears the interrupt by resetting the error-interrupt indicator (SR[ERRI]).
 *
 * Values:
 * - 0 - Do not clear the interrupt.
 * - 1 - Clear the interrupt. When you write 1 to this field, RNGA then resets
 *     the error-interrupt indicator (SR[ERRI]). This bit always reads as 0.
 */
/*@{*/
#define BP_RNG_CR_CLRI       (3U)          /*!< Bit position for RNG_CR_CLRI. */
#define BM_RNG_CR_CLRI       (0x00000008U) /*!< Bit mask for RNG_CR_CLRI. */
#define BS_RNG_CR_CLRI       (1U)          /*!< Bit field size in bits for RNG_CR_CLRI. */

/*! @brief Format value for bitfield RNG_CR_CLRI. */
#define BF_RNG_CR_CLRI(v)    (__REG_VALUE_TYPE((__REG_VALUE_TYPE((v), uint32_t) << BP_RNG_CR_CLRI), uint32_t) & BM_RNG_CR_CLRI)

#ifndef __LANGUAGE_ASM__
/*! @brief Set the CLRI field to a new value. */
#define BW_RNG_CR_CLRI(v)    (BITBAND_ACCESS32(HW_RNG_CR_ADDR, BP_RNG_CR_CLRI) = (v))
#endif
/*@}*/

/*!
 * @name Register RNG_CR, field SLP[4] (RW)
 *
 * Specifies whether RNGA is in Sleep or Normal mode. You can also enter Sleep
 * mode by asserting the DOZE signal.
 *
 * Values:
 * - 0 - Normal mode
 * - 1 - Sleep (low-power) mode
 */
/*@{*/
#define BP_RNG_CR_SLP        (4U)          /*!< Bit position for RNG_CR_SLP. */
#define BM_RNG_CR_SLP        (0x00000010U) /*!< Bit mask for RNG_CR_SLP. */
#define BS_RNG_CR_SLP        (1U)          /*!< Bit field size in bits for RNG_CR_SLP. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_CR_SLP field. */
#define BR_RNG_CR_SLP        (BITBAND_ACCESS32(HW_RNG_CR_ADDR, BP_RNG_CR_SLP))
#endif

/*! @brief Format value for bitfield RNG_CR_SLP. */
#define BF_RNG_CR_SLP(v)     (__REG_VALUE_TYPE((__REG_VALUE_TYPE((v), uint32_t) << BP_RNG_CR_SLP), uint32_t) & BM_RNG_CR_SLP)

#ifndef __LANGUAGE_ASM__
/*! @brief Set the SLP field to a new value. */
#define BW_RNG_CR_SLP(v)     (BITBAND_ACCESS32(HW_RNG_CR_ADDR, BP_RNG_CR_SLP) = (v))
#endif
/*@}*/

/*******************************************************************************
 * HW_RNG_SR - RNGA Status Register
 ******************************************************************************/

#ifndef __LANGUAGE_ASM__
/*!
 * @brief HW_RNG_SR - RNGA Status Register (RO)
 *
 * Reset value: 0x00010000U
 *
 * Indicates the status of RNGA. This register is read-only.
 */
typedef union _hw_rng_sr
{
    uint32_t U;
    struct _hw_rng_sr_bitfields
    {
        uint32_t SECV : 1;             /*!< [0] Security Violation */
        uint32_t LRS : 1;              /*!< [1] Last Read Status */
        uint32_t ORU : 1;              /*!< [2] Output Register Underflow */
        uint32_t ERRI : 1;             /*!< [3] Error Interrupt */
        uint32_t SLP : 1;              /*!< [4] Sleep */
        uint32_t RESERVED0 : 3;        /*!< [7:5]  */
        uint32_t OREG_LVL : 8;         /*!< [15:8] Output Register Level */
        uint32_t OREG_SIZE : 8;        /*!< [23:16] Output Register Size */
        uint32_t RESERVED1 : 8;        /*!< [31:24]  */
    } B;
} hw_rng_sr_t;
#endif

/*!
 * @name Constants and macros for entire RNG_SR register
 */
/*@{*/
#define HW_RNG_SR_ADDR           (REGS_RNG_BASE + 0x4U)

#ifndef __LANGUAGE_ASM__
#define HW_RNG_SR                (*(__I hw_rng_sr_t *) HW_RNG_SR_ADDR)
#define HW_RNG_SR_RD()           (HW_RNG_SR.U)
#endif
/*@}*/

/*
 * Constants & macros for individual RNG_SR bitfields
 */

/*!
 * @name Register RNG_SR, field SECV[0] (RO)
 *
 * Used only when high assurance is enabled (CR[HA]). Indicates that a security
 * violation has occurred.This field is sticky. To clear SR[SECV], you must reset
 * RNGA.
 *
 * Values:
 * - 0 - No security violation
 * - 1 - Security violation
 */
/*@{*/
#define BP_RNG_SR_SECV       (0U)          /*!< Bit position for RNG_SR_SECV. */
#define BM_RNG_SR_SECV       (0x00000001U) /*!< Bit mask for RNG_SR_SECV. */
#define BS_RNG_SR_SECV       (1U)          /*!< Bit field size in bits for RNG_SR_SECV. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_SR_SECV field. */
#define BR_RNG_SR_SECV       (BITBAND_ACCESS32(HW_RNG_SR_ADDR, BP_RNG_SR_SECV))
#endif
/*@}*/

/*!
 * @name Register RNG_SR, field LRS[1] (RO)
 *
 * Indicates whether the most recent read of OR[RANDOUT] caused an OR underflow
 * condition, regardless of whether the error interrupt is masked (CR[INTM]). An
 * OR underflow condition occurs when you read OR[RANDOUT] and SR[OREG_LVL]=0.
 * After you read this register, RNGA writes 0 to this field.
 *
 * Values:
 * - 0 - No underflow
 * - 1 - Underflow
 */
/*@{*/
#define BP_RNG_SR_LRS        (1U)          /*!< Bit position for RNG_SR_LRS. */
#define BM_RNG_SR_LRS        (0x00000002U) /*!< Bit mask for RNG_SR_LRS. */
#define BS_RNG_SR_LRS        (1U)          /*!< Bit field size in bits for RNG_SR_LRS. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_SR_LRS field. */
#define BR_RNG_SR_LRS        (BITBAND_ACCESS32(HW_RNG_SR_ADDR, BP_RNG_SR_LRS))
#endif
/*@}*/

/*!
 * @name Register RNG_SR, field ORU[2] (RO)
 *
 * Indicates whether an OR underflow condition has occurred since you last read
 * this register (SR) or RNGA was reset, regardless of whether the error
 * interrupt is masked (CR[INTM]). An OR underflow condition occurs when you read
 * OR[RANDOUT] and SR[OREG_LVL]=0. After you read this register, RNGA writes 0 to this
 * field.
 *
 * Values:
 * - 0 - No underflow
 * - 1 - Underflow
 */
/*@{*/
#define BP_RNG_SR_ORU        (2U)          /*!< Bit position for RNG_SR_ORU. */
#define BM_RNG_SR_ORU        (0x00000004U) /*!< Bit mask for RNG_SR_ORU. */
#define BS_RNG_SR_ORU        (1U)          /*!< Bit field size in bits for RNG_SR_ORU. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_SR_ORU field. */
#define BR_RNG_SR_ORU        (BITBAND_ACCESS32(HW_RNG_SR_ADDR, BP_RNG_SR_ORU))
#endif
/*@}*/

/*!
 * @name Register RNG_SR, field ERRI[3] (RO)
 *
 * Indicates whether an OR underflow condition has occurred since you last
 * cleared the error interrupt (CR[CLRI]) or RNGA was reset, regardless of whether the
 * error interrupt is masked (CR[INTM]). An OR underflow condition occurs when
 * you read OR[RANDOUT] and SR[OREG_LVL]=0. After you reset the error-interrupt
 * indicator (via CR[CLRI]), RNGA writes 0 to this field.
 *
 * Values:
 * - 0 - No underflow
 * - 1 - Underflow
 */
/*@{*/
#define BP_RNG_SR_ERRI       (3U)          /*!< Bit position for RNG_SR_ERRI. */
#define BM_RNG_SR_ERRI       (0x00000008U) /*!< Bit mask for RNG_SR_ERRI. */
#define BS_RNG_SR_ERRI       (1U)          /*!< Bit field size in bits for RNG_SR_ERRI. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_SR_ERRI field. */
#define BR_RNG_SR_ERRI       (BITBAND_ACCESS32(HW_RNG_SR_ADDR, BP_RNG_SR_ERRI))
#endif
/*@}*/

/*!
 * @name Register RNG_SR, field SLP[4] (RO)
 *
 * Specifies whether RNGA is in Sleep or Normal mode. You can also enter Sleep
 * mode by asserting the DOZE signal.
 *
 * Values:
 * - 0 - Normal mode
 * - 1 - Sleep (low-power) mode
 */
/*@{*/
#define BP_RNG_SR_SLP        (4U)          /*!< Bit position for RNG_SR_SLP. */
#define BM_RNG_SR_SLP        (0x00000010U) /*!< Bit mask for RNG_SR_SLP. */
#define BS_RNG_SR_SLP        (1U)          /*!< Bit field size in bits for RNG_SR_SLP. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_SR_SLP field. */
#define BR_RNG_SR_SLP        (BITBAND_ACCESS32(HW_RNG_SR_ADDR, BP_RNG_SR_SLP))
#endif
/*@}*/

/*!
 * @name Register RNG_SR, field OREG_LVL[15:8] (RO)
 *
 * Indicates the number of random-data words that are in OR[RANDOUT], which
 * indicates whether OR[RANDOUT] is valid.If you read OR[RANDOUT] when SR[OREG_LVL]
 * is not 0, then the contents of a random number contained in OR[RANDOUT] are
 * returned, and RNGA writes 0 to both OR[RANDOUT] and SR[OREG_LVL].
 *
 * Values:
 * - 0 - No words (empty)
 * - 1 - One word (valid)
 */
/*@{*/
#define BP_RNG_SR_OREG_LVL   (8U)          /*!< Bit position for RNG_SR_OREG_LVL. */
#define BM_RNG_SR_OREG_LVL   (0x0000FF00U) /*!< Bit mask for RNG_SR_OREG_LVL. */
#define BS_RNG_SR_OREG_LVL   (8U)          /*!< Bit field size in bits for RNG_SR_OREG_LVL. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_SR_OREG_LVL field. */
#define BR_RNG_SR_OREG_LVL   (HW_RNG_SR.B.OREG_LVL)
#endif
/*@}*/

/*!
 * @name Register RNG_SR, field OREG_SIZE[23:16] (RO)
 *
 * Indicates the size of the Output (OR) register in terms of the number of
 * 32-bit random-data words it can hold.
 *
 * Values:
 * - 1 - One word (this value is fixed)
 */
/*@{*/
#define BP_RNG_SR_OREG_SIZE  (16U)         /*!< Bit position for RNG_SR_OREG_SIZE. */
#define BM_RNG_SR_OREG_SIZE  (0x00FF0000U) /*!< Bit mask for RNG_SR_OREG_SIZE. */
#define BS_RNG_SR_OREG_SIZE  (8U)          /*!< Bit field size in bits for RNG_SR_OREG_SIZE. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_SR_OREG_SIZE field. */
#define BR_RNG_SR_OREG_SIZE  (HW_RNG_SR.B.OREG_SIZE)
#endif
/*@}*/

/*******************************************************************************
 * HW_RNG_ER - RNGA Entropy Register
 ******************************************************************************/

#ifndef __LANGUAGE_ASM__
/*!
 * @brief HW_RNG_ER - RNGA Entropy Register (WORZ)
 *
 * Reset value: 0x00000000U
 *
 * Specifies an entropy value that RNGA uses in addition to its ring oscillators
 * to seed its pseudorandom algorithm. This is a write-only register; reads
 * return all zeros.
 */
typedef union _hw_rng_er
{
    uint32_t U;
    struct _hw_rng_er_bitfields
    {
        uint32_t EXT_ENT : 32;         /*!< [31:0] External Entropy */
    } B;
} hw_rng_er_t;
#endif

/*!
 * @name Constants and macros for entire RNG_ER register
 */
/*@{*/
#define HW_RNG_ER_ADDR           (REGS_RNG_BASE + 0x8U)

#ifndef __LANGUAGE_ASM__
#define HW_RNG_ER                (*(__O hw_rng_er_t *) HW_RNG_ER_ADDR)
#define HW_RNG_ER_RD()           (HW_RNG_ER.U)
#define HW_RNG_ER_WR(v)          (HW_RNG_ER.U = (v))
#endif
/*@}*/

/*
 * Constants & macros for individual RNG_ER bitfields
 */

/*!
 * @name Register RNG_ER, field EXT_ENT[31:0] (WORZ)
 *
 * Specifies an entropy value that RNGA uses in addition to its ring oscillators
 * to seed its pseudorandom algorithm.Specifying a value for this field is
 * optional but recommended. You can write to this field at any time during operation.
 */
/*@{*/
#define BP_RNG_ER_EXT_ENT    (0U)          /*!< Bit position for RNG_ER_EXT_ENT. */
#define BM_RNG_ER_EXT_ENT    (0xFFFFFFFFU) /*!< Bit mask for RNG_ER_EXT_ENT. */
#define BS_RNG_ER_EXT_ENT    (32U)         /*!< Bit field size in bits for RNG_ER_EXT_ENT. */

/*! @brief Format value for bitfield RNG_ER_EXT_ENT. */
#define BF_RNG_ER_EXT_ENT(v) (__REG_VALUE_TYPE((__REG_VALUE_TYPE((v), uint32_t) << BP_RNG_ER_EXT_ENT), uint32_t) & BM_RNG_ER_EXT_ENT)

#ifndef __LANGUAGE_ASM__
/*! @brief Set the EXT_ENT field to a new value. */
#define BW_RNG_ER_EXT_ENT(v) (HW_RNG_ER_WR(v))
#endif
/*@}*/

/*******************************************************************************
 * HW_RNG_OR - RNGA Output Register
 ******************************************************************************/

#ifndef __LANGUAGE_ASM__
/*!
 * @brief HW_RNG_OR - RNGA Output Register (RO)
 *
 * Reset value: 0x00000000U
 *
 * Stores a random-data word generated by RNGA.
 */
typedef union _hw_rng_or
{
    uint32_t U;
    struct _hw_rng_or_bitfields
    {
        uint32_t RANDOUT : 32;         /*!< [31:0] Random Output */
    } B;
} hw_rng_or_t;
#endif

/*!
 * @name Constants and macros for entire RNG_OR register
 */
/*@{*/
#define HW_RNG_OR_ADDR           (REGS_RNG_BASE + 0xCU)

#ifndef __LANGUAGE_ASM__
#define HW_RNG_OR                (*(__I hw_rng_or_t *) HW_RNG_OR_ADDR)
#define HW_RNG_OR_RD()           (HW_RNG_OR.U)
#endif
/*@}*/

/*
 * Constants & macros for individual RNG_OR bitfields
 */

/*!
 * @name Register RNG_OR, field RANDOUT[31:0] (RO)
 *
 * Stores a random-data word generated by RNGA. This is a read-only field.Before
 * reading RANDOUT, be sure it is valid (SR[OREG_LVL]=1).
 *
 * Values:
 * - 0 - Invalid data (if you read this field when it is 0 and SR[OREG_LVL] is
 *     0, RNGA then writes 1 to SR[ERRI], SR[ORU], and SR[LRS]; when the error
 *     interrupt is not masked (CR[INTM]=0), RNGA also asserts an error interrupt
 *     request to the interrupt controller).
 */
/*@{*/
#define BP_RNG_OR_RANDOUT    (0U)          /*!< Bit position for RNG_OR_RANDOUT. */
#define BM_RNG_OR_RANDOUT    (0xFFFFFFFFU) /*!< Bit mask for RNG_OR_RANDOUT. */
#define BS_RNG_OR_RANDOUT    (32U)         /*!< Bit field size in bits for RNG_OR_RANDOUT. */

#ifndef __LANGUAGE_ASM__
/*! @brief Read current value of the RNG_OR_RANDOUT field. */
#define BR_RNG_OR_RANDOUT    (HW_RNG_OR.U)
#endif
/*@}*/

/*******************************************************************************
 * hw_rng_t - module struct
 ******************************************************************************/
/*!
 * @brief All RNG module registers.
 */
#ifndef __LANGUAGE_ASM__
#pragma pack(1)
typedef struct _hw_rng
{
    __IO hw_rng_cr_t CR;                   /*!< [0x0] RNGA Control Register */
    __I hw_rng_sr_t SR;                    /*!< [0x4] RNGA Status Register */
    __O hw_rng_er_t ER;                    /*!< [0x8] RNGA Entropy Register */
    __I hw_rng_or_t OR;                    /*!< [0xC] RNGA Output Register */
} hw_rng_t;
#pragma pack()

/*! @brief Macro to access all RNG registers. */
/*! @return Reference (not a pointer) to the registers struct. To get a pointer to the struct,
 *     use the '&' operator, like <code>&HW_RNG</code>. */
#define HW_RNG         (*(hw_rng_t *) REGS_RNG_BASE)
#endif

#endif /* __HW_RNG_REGISTERS_H__ */
/* v22/130726/0.9 */
/* EOF */
