Camera Capture Image Example
===================

This is a simple example for camera capture image. To use this example, download `camera_capture_flash.pac` by ResearchDownload.

There are two cases swith every 3 seconds:

- ContinuousFrame mode

- OneShotFrame

### INIT CAMERA

drvCamInit->find sensor in system

drvCamGetSensorInfo->get sensor info

drvCamPowerOn->init sensor by i2c

### SET WORK MODE

#### ContinuousFrame mode

drvCamStartPreview -> start ping-pong buffer

drvCamPreviewDQBUF->get one frame buffer from drv

XXX DO YOU WORK

drvCamPreviewQBUF->release frame buffer to drv

drvCamStopPreview->stop cam data irq

#### OneShotFrame

drvCamCaptureImage->get one frame data

### CLOSE CAMAERA

drvCamClose