/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#ifndef _SPI_H_
#define _SPI_H_

#ifdef CT_ASM
#error "You are trying to use in an assembly code the normal H description of 'spi'."
#endif

// =============================================================================
//  MACROS
// =============================================================================
#define SPI_TX_FIFO_SIZE (16)
#define SPI_RX_FIFO_SIZE (16)

// =============================================================================
//  TYPES
// =============================================================================

// ============================================================================
// SPI_T
// -----------------------------------------------------------------------------
///
// =============================================================================
#define REG_SPI1_BASE 0x08812000
#define REG_SPI2_BASE 0x08813000

typedef volatile struct
{
    REG32 ctrl;   //0x00000000
    REG32 status; //0x00000004
    /// Spi1 fifo size (rxtx_buffer): 8bits.
    /// Spi2 fifo size (rxtx_buffer): 8bits.
    /// Spi3 fifo size (rxtx_buffer): 32bits.
    ///
    REG32 rxtx_buffer; //0x00000008
    REG32 cfg;         //0x0000000C
    REG32 pattern;     //0x00000010
    REG32 stream;      //0x00000014
    REG32 pin_control; //0x00000018
    REG32 irq;         //0x0000001C
} HWP_SPI_T;

#define hwp_spi1 ((HWP_SPI_T *)REG_ACCESS_ADDRESS(REG_SPI1_BASE))
#define hwp_spi2 ((HWP_SPI_T *)REG_ACCESS_ADDRESS(REG_SPI2_BASE))

//ctrl
typedef union {
    REG32 v;
    struct
    {
        REG32 enable : 1; // [0]
        REG32 cs_sel : 2; // [2:1]
        REG32 __3_3 : 1;
        REG32 input_mode : 1;     // [4]
        REG32 clock_polarity : 1; // [5]
        REG32 clock_delay : 2;    // [7:6]
        REG32 do_delay : 2;       // [9:8]
        REG32 di_delay : 2;       // [11:10]
        REG32 cs_delay : 2;       // [13:12]
        REG32 cs_pulse : 2;       // [15:14]
        REG32 frame_size : 5;     // [20:16]
        REG32 __23_21 : 3;
        REG32 oe_delay : 5;          // [28:24]
        REG32 ctrl_data_mux_sel : 1; // [29]
        REG32 input_sel : 2;         // [31:30]
    } b;
} REG_SPI_CTRL_T;

//status
typedef union {
    REG32 v;
    struct
    {
        REG32 active_status : 1; // [0], read only
        REG32 __2_1 : 2;
        REG32 cause_rx_ovf_irq : 1; // [3]
        REG32 cause_tx_th_irq : 1;  // [4], read only
        REG32 cause_tx_dma_irq : 1; // [5]
        REG32 cause_rx_th_irq : 1;  // [6], read only
        REG32 cause_rx_dma_irq : 1; // [7]
        REG32 __8_8 : 1;
        REG32 tx_ovf : 1;      // [9]
        REG32 rx_udf : 1;      // [10]
        REG32 rx_ovf : 1;      // [11]
        REG32 tx_th : 1;       // [12], read only
        REG32 tx_dma_done : 1; // [13]
        REG32 rx_th : 1;       // [14], read only
        REG32 rx_dma_done : 1; // [15]
        REG32 tx_space : 5;    // [20:16], read only
        REG32 __23_21 : 3;
        REG32 rx_level : 5; // [28:24], read only
        REG32 __29_29 : 1;
        REG32 fifo_flush : 1; // [30]
        REG32 __31_31 : 1;
    } b;
} REG_SPI_STATUS_T;

//rxtx_buffer
typedef union {
    REG32 v;
    struct
    {
        REG32 data_in_out : 29; // [28:0]
        REG32 cs : 2;           // [30:29]
        REG32 read_ena : 1;     // [31]
    } b;
} REG_SPI_RXTX_BUFFER_T;

//cfg
typedef union {
    REG32 v;
    struct
    {
        REG32 cs_polarity_0 : 1; // [0]
        REG32 cs_polarity_1 : 1; // [1]
        REG32 cs_polarity_2 : 1; // [2]
        REG32 __15_3 : 13;
        REG32 clock_divider : 10; // [25:16]
        REG32 __27_26 : 2;
        REG32 clock_limiter : 1; // [28]
        REG32 __31_29 : 3;
    } b;
} REG_SPI_CFG_T;

//pattern
typedef union {
    REG32 v;
    struct
    {
        REG32 pattern : 8;          // [7:0]
        REG32 pattern_mode : 1;     // [8]
        REG32 pattern_selector : 1; // [9]
        REG32 __31_10 : 22;
    } b;
} REG_SPI_PATTERN_T;

//stream
typedef union {
    REG32 v;
    struct
    {
        REG32 tx_stream_bit : 1; // [0]
        REG32 __7_1 : 7;
        REG32 tx_stream_mode : 1; // [8]
        REG32 __15_9 : 7;
        REG32 tx_stream_stop_with_rx_dma_done : 1; // [16]
        REG32 __31_17 : 15;
    } b;
} REG_SPI_STREAM_T;

//pin_control
typedef union {
    REG32 v;
    struct
    {
        REG32 clk_ctrl : 2; // [1:0]
        REG32 do_ctrl : 2;  // [3:2]
        REG32 cs0_ctrl : 2; // [5:4]
        REG32 cs1_ctrl : 2; // [7:6]
        REG32 cs2_ctrl : 2; // [9:8]
        REG32 __31_10 : 22;
    } b;
} REG_SPI_PIN_CONTROL_T;

//irq
typedef union {
    REG32 v;
    struct
    {
        REG32 mask_rx_ovf_irq : 1; // [0]
        REG32 mask_tx_th_irq : 1;  // [1]
        REG32 mask_tx_dma_irq : 1; // [2]
        REG32 mask_rx_th_irq : 1;  // [3]
        REG32 mask_rx_dma_irq : 1; // [4]
        REG32 tx_threshold : 2;    // [6:5]
        REG32 rx_threshold : 2;    // [8:7]
        REG32 __31_9 : 23;
    } b;
} REG_SPI_IRQ_T;

//ctrl
#define SPI_CS_SEL_V_CS0 (0)
#define SPI_CS_SEL_V_CS1 (1)
#define SPI_CS_SEL_V_CS2 (2)
#define SPI_CS_SEL_V_CS3 (3)
#define SPI_CTRL_DATA_MUX_SEL_V_CTRL_REG_SEL (0)
#define SPI_CTRL_DATA_MUX_SEL_V_DATA_REG_SEL (1)

//cfg
#define SPI_CS_POLARITY_0_V_ACTIVE_HIGH (0)
#define SPI_CS_POLARITY_0_V_ACTIVE_LOW (1)
#define SPI_CS_POLARITY_1_V_ACTIVE_HIGH (0)
#define SPI_CS_POLARITY_1_V_ACTIVE_LOW (1)
#define SPI_CS_POLARITY_2_V_ACTIVE_HIGH (0)
#define SPI_CS_POLARITY_2_V_ACTIVE_LOW (1)

//pattern
#define SPI_PATTERN_MODE_V_DISABLED (0)
#define SPI_PATTERN_MODE_V_ENABLED (1)
#define SPI_PATTERN_SELECTOR_V_UNTIL (0)
#define SPI_PATTERN_SELECTOR_V_WHILE (1)

//stream
#define SPI_TX_STREAM_BIT_V_ZERO (0)
#define SPI_TX_STREAM_BIT_V_ONE (1)
#define SPI_TX_STREAM_MODE_V_DISABLED (0)
#define SPI_TX_STREAM_MODE_V_ENABLED (1)
#define SPI_TX_STREAM_STOP_WITH_RX_DMA_DONE_V_DISABLED (0)
#define SPI_TX_STREAM_STOP_WITH_RX_DMA_DONE_V_ENABLED (1)

//pin_control
#define SPI_CLK_CTRL_V_SPI_CTRL (0)
#define SPI_CLK_CTRL_V_INPUT_CTRL (1)
#define SPI_CLK_CTRL_V_FORCE_0_CTRL (2)
#define SPI_CLK_CTRL_V_FORCE_1_CTRL (3)
#define SPI_DO_CTRL_V_SPI_CTRL (0)
#define SPI_DO_CTRL_V_INPUT_CTRL (1)
#define SPI_DO_CTRL_V_FORCE_0_CTRL (2)
#define SPI_DO_CTRL_V_FORCE_1_CTRL (3)
#define SPI_CS0_CTRL_V_SPI_CTRL (0)
#define SPI_CS0_CTRL_V_INPUT_CTRL (1)
#define SPI_CS0_CTRL_V_FORCE_0_CTRL (2)
#define SPI_CS0_CTRL_V_FORCE_1_CTRL (3)
#define SPI_CS1_CTRL_V_SPI_CTRL (0)
#define SPI_CS1_CTRL_V_INPUT_CTRL (1)
#define SPI_CS1_CTRL_V_FORCE_0_CTRL (2)
#define SPI_CS1_CTRL_V_FORCE_1_CTRL (3)
#define SPI_CS2_CTRL_V_SPI_CTRL (0)
#define SPI_CS2_CTRL_V_INPUT_CTRL (1)
#define SPI_CS2_CTRL_V_FORCE_0_CTRL (2)
#define SPI_CS2_CTRL_V_FORCE_1_CTRL (3)

//irq
#define SPI_TX_THRESHOLD_V_1_EMPTY_SLOT (0)
#define SPI_TX_THRESHOLD_V_4_EMPTY_SLOTS (1)
#define SPI_TX_THRESHOLD_V_8_EMPTY_SLOTS (2)
#define SPI_TX_THRESHOLD_V_12_EMPTY_SLOTS (3)
#define SPI_RX_THRESHOLD_V_1_VALID_DATA (0)
#define SPI_RX_THRESHOLD_V_4_VALID_DATA (1)
#define SPI_RX_THRESHOLD_V_8_VALID_DATA (2)
#define SPI_RX_THRESHOLD_V_12_VALID_DATA (3)

#endif
