Camera Preview Display Example
===================

This is a simple example for camera preview and display on lcd. To use this example, download `camera_prev_display.pac` by ResearchDownload.

There is one case that dispaly images on lcd when the DMA transport is done.

### INIT CAMERA

drvCamInit->find sensor in system

drvCamGetSensorInfo->get sensor info

drvCamPowerOn->init sensor by i2c

### INTI LCD

halPmuSwitchPower -> open lcd powers

drvLcdInit->init lcd

drvLcdGetLcdInfo->get information from lcd

prvBlackPrint-> show a black image on lcd

prvCameraPrint-> show images on lcd

XXX DO YOU WORK

drvCamStartPreview->into camera preview mode

drvCamPreviewDQBUF->get an image

drvCamPreviewQBUF->release frame buffer to drv