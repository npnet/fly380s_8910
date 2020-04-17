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
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_ERROR

#include "osi_log.h"
#include "osi_api.h"
#include "hwregs.h"
#include <stdlib.h>
#include <string.h>
#include "hal_chip.h"
#include "drv_i2c.h"
#include "drv_camera.h"
#include "drv_names.h"
#include "drv_ifc.h"
#include "image_sensor.h"
#include "drv_uart.h"

drvIfcChannel_t camp_ifc;

static void prvCamCtrlEnable(bool enable)
{
    REG_CAMERA_CTRL_T ctrl;
    ctrl.v = hwp_camera->ctrl;

    if (enable)
    {
        ctrl.b.enable = 1;
    }
    else
    {
        ctrl.b.enable = 0;
    }
    hwp_camera->ctrl = ctrl.v;
}

static void prvCamSpiRegInit(void)
{
    hwp_camera->spi_camera_reg0 = 0;
    hwp_camera->spi_camera_reg1 = 0;
    hwp_camera->spi_camera_reg2 = 0;
    hwp_camera->spi_camera_reg3 = 0;
    hwp_camera->spi_camera_reg4 = 0;
    hwp_camera->spi_camera_reg5 = 0;
    hwp_camera->spi_camera_reg6 = 0;

    hwp_camera->spi_camera_reg5 = 0x00ffffff;
    hwp_camera->spi_camera_reg6 = 0x01000240;
}

static void prvCtrlDataFormat(uint8_t type)
{
    REG_CAMERA_CTRL_T ctrl;
    ctrl.v = hwp_camera->ctrl;
    ctrl.b.dataformat = type;

    hwp_camera->ctrl = ctrl.v;
}

void drvCamSetMCLK(cameraClk_t Mclk)
{
    REG_SYS_CTRL_CLK_AP_APB_ENABLE_T clk_ap_apb_enable;
    REG_SYS_CTRL_CLK_OTHERS_ENABLE_T clk_others_enable;
    REG_SYS_CTRL_SEL_CLOCK_T sel_clock;

    hwp_sysCtrl->reg_dbg = SYS_CTRL_PROTECT_UNLOCK; // unlock
    clk_ap_apb_enable.v = hwp_sysCtrl->clk_ap_apb_enable;
    clk_ap_apb_enable.b.enable_ap_apb_clk_id_mod_camera = SYS_CTRL_MODE_AP_APB_CLK_ID_MOD_CAMERA_V_MANUAL;
    clk_ap_apb_enable.b.enable_ap_apb_clk_id_camera = SYS_CTRL_MODE_AP_APB_CLK_ID_CAMERA_V_MANUAL;
    hwp_sysCtrl->clk_ap_apb_enable = clk_ap_apb_enable.v;

    clk_others_enable.v = hwp_sysCtrl->clk_others_enable;
    clk_others_enable.b.enable_oc_id_pix = SYS_CTRL_MODE_OC_ID_PIX_V_MANUAL;
    clk_others_enable.b.enable_oc_id_isp = SYS_CTRL_MODE_OC_ID_ISP_V_MANUAL;
    clk_others_enable.b.enable_oc_id_spicam = SYS_CTRL_MODE_OC_ID_SPICAM_V_MANUAL;
    clk_others_enable.b.enable_oc_id_cam = SYS_CTRL_MODE_OC_ID_CAM_V_MANUAL;
    hwp_sysCtrl->clk_others_enable = clk_others_enable.v;

    sel_clock.v = hwp_sysCtrl->sel_clock;
    sel_clock.b.sys_sel_fast = SYS_CTRL_SYS_SEL_FAST_V_FAST;
    hwp_sysCtrl->sel_clock = sel_clock.v;
    osiDelayUS(10);

    halCameraClockRequest(CLK_CAMA_USER_CAMERA, Mclk);

    osiDelayUS(40);
}

void drvCamDisableMCLK(void)
{
    REG_SYS_CTRL_CLK_AP_APB_DISABLE_T clk_ap_apb_disable;
    // REG_SYS_CTRL_CFG_CLK_CAMERA_OUT_T cfg_clk_camera_out;
    REG_SYS_CTRL_CLK_OTHERS_DISABLE_T clk_others_disable;

    hwp_sysCtrl->reg_dbg = SYS_CTRL_PROTECT_UNLOCK;
    clk_ap_apb_disable.v = 0;
    clk_ap_apb_disable.b.disable_ap_apb_clk_id_mod_camera = 1;
    hwp_sysCtrl->clk_ap_apb_disable = clk_ap_apb_disable.v;

    clk_others_disable.v = 0;
    clk_others_disable.b.disable_oc_id_pix = 1;
    clk_others_disable.b.disable_oc_id_isp = 1;
    clk_others_disable.b.disable_oc_id_spicam = 1;
    clk_others_disable.b.disable_oc_id_cam = 1;
    hwp_sysCtrl->clk_others_disable = clk_others_disable.v;

    halCameraClockRelease(CLK_CAMA_USER_CAMERA);
    /*
    hwp_sysCtrl->clk_ap_apb_disable = 1 << 1;
    hwp_sysCtrl->clk_others_disable = 1 << 10;
    hwp_sysCtrl->clk_others_disable = 1 << 12;
    hwp_sysCtrl->clk_others_disable = (1 << 19 | 1 << 20);
    hwp_sysCtrl->cfg_clk_camera_out = 0 << 0;
	*/
    osiDelayUS(30);
}

void drvCamInitIfc(void)
{
    if (drvIfcChannelInit(&camp_ifc, DRV_NAME_CAMERA1, DRV_IFC_RX))
    {
        drvIfcRequestChannel(&camp_ifc);
    }
}

bool drvWaitCamIfcDone(void)
{
    int64_t startTime = osiUpTimeUS();

    while (!(drvIfcGetTC(&camp_ifc) == 0))
    {
        if (osiUpTimeUS() - startTime > 1000000)
        {
            OSI_LOGE(0, "READ timeout\n");
            return false;
        }
    }
    return true;
}

void drvCampStartTransfer(uint32_t size, uint8_t *data)
{
    uint32_t critical = osiEnterCritical();
    drvIfcFlush(&camp_ifc);
    osiDCacheInvalidate(data, size);
    drvIfcStart(&camp_ifc, data, size);
    osiExitCritical(critical);
}

bool drvCampStopTransfer(uint32_t size, uint8_t *data)
{
    //drvIfcStop(&camp_ifc);

    uint32_t tc = 0;
    uint32_t critical = osiEnterCritical();
    tc = drvIfcGetTC(&camp_ifc);
    if (tc != 0)
    {
        drvCameraControllerEnable(false);
        drvIfcFlush(&camp_ifc);
        while (!drvIfcIsFifoEmpty(&camp_ifc))
            ;
        osiExitCritical(critical);
        drvCameraControllerEnable(true);
        return false;
    }
    osiDCacheInvalidate(data, size);
    osiExitCritical(critical);
    if (!(hwp_camera->irq_cause & (1 << 0)))
    {
        return true;
    }
    return false;
}

void drvCampIfcDeinit(void)
{
    drvIfcReleaseChannel(&camp_ifc);
}

void drvCamSetIrqHandler(osiIrqHandler_t hanlder, void *ctx)
{
    uint32_t irqn = HAL_SYSIRQ_NUM(SYS_IRQ_ID_CAMERA);

    osiIrqDisable(irqn);
    osiIrqSetHandler(irqn, hanlder, ctx);
    osiIrqSetPriority(irqn, SYS_IRQ_PRIO_CAMERA);
    osiIrqEnable(irqn);
}

void drvCamDisableIrq(void)
{
    uint32_t irqn = HAL_SYSIRQ_NUM(SYS_IRQ_ID_CAMERA);
    osiIrqDisable(irqn);
}

void drvCamCmdSetFifoRst(void)
{
    REG_CAMERA_CMD_SET_T cmd_set;
    cmd_set.v = hwp_camera->cmd_set;
    cmd_set.b.fifo_reset = 1; //
    hwp_camera->cmd_set = cmd_set.v;
}

void drvCamSetIrqMask(cameraIrqCause_t mask)
{
    REG_CAMERA_IRQ_MASK_T irq_mask;

    irq_mask.v = hwp_camera->irq_mask;
    irq_mask.b.ovfl = mask.overflow;
    irq_mask.b.vsync_r = mask.fstart;
    irq_mask.b.vsync_f = mask.fend;
    irq_mask.b.dma_done = mask.dma;

    hwp_camera->irq_mask = irq_mask.v;
}
void drvCamClrIrqMask(void)
{
    REG_CAMERA_IRQ_MASK_T irq_mask;

    irq_mask.v = hwp_camera->irq_mask;
    irq_mask.b.ovfl = 0;
    irq_mask.b.vsync_f = 0;
    irq_mask.b.vsync_r = 0;
    irq_mask.b.dma_done = 0;

    hwp_camera->irq_mask = irq_mask.v;
}

void drvCampRegInit(sensorInfo_t *config)
{
    REG_CAMERA_CTRL_T campCtrl;
    //REG_CAMERA_SCL_CONFIG_T campSclCfg;
    REG_CAMERA_DSTWINCOL_T campCol;
    REG_CAMERA_DSTWINROW_T campRow;
    REG_SYS_CTRL_SEL_CLOCK_T sel_clock;
    REG_SYS_CTRL_CFG_PLL_PIX_DIV_T cfg_pll_pix_div;
    REG_SYS_CTRL_CLK_AP_APB_MODE_T clk_ap_apb_mode;
    REG_SYS_CTRL_CLK_OTHERS_ENABLE_T clk_others_enable;

    campCtrl.v = hwp_camera->ctrl;
    //campSclCfg.v = hwp_camera->scl_config;
    campCol.v = hwp_camera->dstwincol;
    campRow.v = hwp_camera->dstwinrow;
    sel_clock.v = hwp_sysCtrl->sel_clock;
    cfg_pll_pix_div.v = hwp_sysCtrl->cfg_pll_pix_div;
    clk_ap_apb_mode.v = hwp_sysCtrl->clk_ap_apb_mode;
    clk_others_enable.v = hwp_sysCtrl->clk_others_enable;

    if (config->scaleEnable)
    {
        campCtrl.b.decimrow = 0;
        campCtrl.b.decimcol = 0;
        /*
		campSclCfg.v = hwp_camera->scl_config;
		campSclCfg.b.scale_row = (config->rowRatio&3);
		campSclCfg.b.scale_col = (config->colRatio&3);
		campSclCfg.b.scale_en = 1;
		campSclCfg.b.data_out_swap = 1;
		hwp_camera->scl_config = campSclCfg.v;
		*/
    }
    else
    {
        /*
		hwp_camera->scl_config = 0;
		campSclCfg.b.data_out_swap = 1;
		hwp_camera->scl_config = campSclCfg.v;
		*/
        switch (config->rowRatio)
        {
        case ROW_RATIO_1_1:
            campCtrl.b.decimrow = 0;
            break;
        case ROW_RATIO_1_2:
            campCtrl.b.decimrow = 1;
            break;
        case ROW_RATIO_1_3:
            campCtrl.b.decimrow = 2;
            break;
        case ROW_RATIO_1_4:
            campCtrl.b.decimrow = 3;
            break;
        default:
            break;
        }
        switch (config->colRatio)
        {
        case ROW_RATIO_1_1:
            campCtrl.b.decimrow = 0;
            break;
        case ROW_RATIO_1_2:
            campCtrl.b.decimrow = 1;
            break;
        case ROW_RATIO_1_3:
            campCtrl.b.decimrow = 2;
            break;
        case ROW_RATIO_1_4:
            campCtrl.b.decimrow = 3;
            break;
        default:
            break;
        }
    }
    campCtrl.b.dataformat = 1;
    hwp_camera->ctrl = campCtrl.v;
    if (config->cropEnable)
    {
        campCtrl.v = hwp_camera->ctrl;
        campCtrl.b.cropen = 1;
        hwp_camera->ctrl = campCtrl.v;
        campCol.b.dstwincolstart = config->dstWinColStart;
        campCol.b.dstwincolend = config->dstWinColEnd;
        hwp_camera->dstwincol = campCol.v;
        campRow.b.dstwinrowstart = config->dstWinRowStart;
        campRow.b.dstwinrowend = config->dstWinRowEnd;
        hwp_camera->dstwinrow = campRow.v;
    }
    if (!config->rstActiveH)
    {
        campCtrl.v = hwp_camera->ctrl;
        campCtrl.b.reset_pol = 1;
        hwp_camera->ctrl = campCtrl.v;
    }
    if (!config->pdnActiveH)
    {
        campCtrl.v = hwp_camera->ctrl;
        campCtrl.b.pwdn_pol = 1;
        hwp_camera->ctrl = campCtrl.v;
    }
    if (config->dropFrame)
    {
        campCtrl.v = hwp_camera->ctrl;
        campCtrl.b.decimfrm = 1;
        hwp_camera->ctrl = campCtrl.v;
    }
    campCtrl.v = hwp_camera->ctrl;
    campCtrl.b.vsync_drop = 0;
    campCtrl.b.vsync_pol = 0;
    hwp_camera->ctrl = campCtrl.v;

    sel_clock.b.camera_clk_pll_sel = 0;
    hwp_sysCtrl->sel_clock = sel_clock.v;
    cfg_pll_pix_div.b.cfg_pll_pix_div_update = 1;
    cfg_pll_pix_div.b.cfg_pll_pix_div = ((42 & 0x7F) << 0);
    hwp_sysCtrl->cfg_pll_pix_div = cfg_pll_pix_div.v;
    prvCamSpiRegInit();
    if (config->camSpiMode != SPI_MODE_NO)
    {
        REG_CAMERA_SPI_CAMERA_REG0_T reg0Cfg;
        REG_CAMERA_SPI_CAMERA_REG1_T reg1Cfg;
        REG_CAMERA_SPI_CAMERA_REG4_T reg4Cfg;

        reg0Cfg.v = hwp_camera->spi_camera_reg0;
        reg1Cfg.v = hwp_camera->spi_camera_reg1;
        reg4Cfg.v = hwp_camera->spi_camera_reg4;

        reg0Cfg.b.line_num_per_frame = config->spi_pixels_per_column;
        reg0Cfg.b.block_num_per_line = config->spi_pixels_per_line >> 1;
        reg0Cfg.b.yuv_out_format = config->camYuvMode;
        reg0Cfg.b.big_end_dis = (config->spi_little_endian_en) ? 1 : 0;
        //reg1Cfg.b.vsync_inv = 0;
        hwp_camera->spi_camera_reg0 = reg0Cfg.v;

        reg1Cfg.b.camera_clk_div_num = config->spi_ctrl_clk_div;
        reg1Cfg.b.sck_ddr_en = 1;
        hwp_camera->spi_camera_reg1 = reg1Cfg.v;
        switch (config->camSpiMode)
        {
        case SPI_MODE_MASTER2_2:
            reg4Cfg.b.camera_spi_master_en_2 = 1;
            reg4Cfg.b.block_num_per_packet = config->spi_pixels_per_line >> 1;
            reg4Cfg.b.sdo_line_choose_bit = 1;
            reg4Cfg.b.data_size_choose_bit = 1;
            reg4Cfg.b.image_height_choose_bit = 1;
            reg4Cfg.b.image_width_choose_bit = 1;
            hwp_camera->spi_camera_reg4 = reg4Cfg.v;
            break;
        default:
            break;
        }
        clk_ap_apb_mode.b.mode_ap_apb_clk_id_camera = 1;
        hwp_sysCtrl->clk_ap_apb_mode = clk_ap_apb_mode.v;
        clk_others_enable.b.enable_oc_id_pix = 1;
        hwp_sysCtrl->clk_others_enable = clk_others_enable.v;
    }
    hwp_camera->csi_enable = 0;
    drvCamSetRst(false);
    drvCamInitIfc();
}

void drvCampRegDeInit(void)
{
    drvCamSetRst(true);
    hwp_camera->spi_camera_reg0 = 0;
    hwp_camera->spi_camera_reg1 = 0;
    hwp_camera->spi_camera_reg4 = 0;

    drvCameraControllerEnable(false);
    drvCampIfcDeinit();
}

void drvCamSetPdn(bool pdnActivH)
{
    REG_CAMERA_CMD_SET_T cmd_set;
    REG_CAMERA_CMD_CLR_T cmd_clr;

    cmd_set.v = hwp_camera->cmd_set;
    cmd_clr.v = hwp_camera->cmd_clr;

    if (pdnActivH)
    {
        cmd_set.b.pwdn = 1;
        hwp_camera->cmd_set = cmd_set.v;
    }
    else
    {
        cmd_clr.b.pwdn = 1;
        hwp_camera->cmd_clr = cmd_clr.v;
    }
}

void drvCamSetRst(bool rstActiveH)
{
    // Configures the Reset bit to the camera
    REG_CAMERA_CMD_SET_T cmd_set;
    REG_CAMERA_CMD_CLR_T cmd_clr;

    cmd_set.v = hwp_camera->cmd_set;
    cmd_clr.v = hwp_camera->cmd_clr;

    if (rstActiveH)
    {
        cmd_set.b.reset = 1;
        hwp_camera->cmd_set = cmd_set.v;
    }
    else
    {
        cmd_clr.b.reset = 1;
        hwp_camera->cmd_clr = cmd_clr.v;
    }
}

void drvCameraControllerEnable(bool enable)
{
    if (enable)
    {
        drvCamCmdSetFifoRst();
        prvCtrlDataFormat(CAMERA_DATAFORMAT_V_YUV422);
        prvCamCtrlEnable(true);
    }
    else
    {
        prvCamCtrlEnable(false);
    }
}
