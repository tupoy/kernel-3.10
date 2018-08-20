/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Header file of Sensor driver
 *
 *
 * Author:
 * -------
 *   PC Huang (MTK02204)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision: 84 $
 * $Modtime:$
 * $Log:$
 *
 * Mar 4 2010 mtk70508
 * [DUMA00154792] Sensor driver
 * 
 *
 * Feb 24 2010 mtk01118
 * [DUMA00025869] [Camera][YUV I/F & Query feature] check in camera code
 * 
 *
 * Aug 5 2009 mtk01051
 * [DUMA00009217] [Camera Driver] CCAP First Check In
 * 
 *
 * Apr 7 2009 mtk02204
 * [DUMA00004012] [Camera] Restructure and rename camera related custom folders and folder name of came
 * 
 *
 * Mar 26 2009 mtk02204
 * [DUMA00003515] [PC_Lint] Remove PC_Lint check warnings of camera related drivers.
 * 
 *
 * Mar 2 2009 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 * 
 *
 * Feb 24 2009 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 * 
 *
 * Dec 27 2008 MTK01813
 * DUMA_MBJ CheckIn Files
 * created by clearfsimport
 *
 * Dec 10 2008 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 * 
 *
 * Oct 27 2008 mtk01051
 * [DUMA00000851] Camera related drivers check in
 * Modify Copyright Header
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H
//#define SIV121D_LOAD_FROM_T_FLASH

#ifdef SIV121DU_LOAD_FROM_T_FLASH
	
		#define SIV121DU_REG_SKIP		0x06
		#define SIV121DU_VAL_SKIP		0x06
	

	/* 0 - Initial value, 1 - Register, 2 - Delay, 3 - End of setting. */
	#define SIV121DU_OP_CODE_INI		0x00		/* Initial value. */
	#define SIV121DU_OP_CODE_REG		0x01		/* Register */
	#define SIV121DU_OP_CODE_DLY		0x02		/* Delay */
	#define SIV121DU_OP_CODE_END		0x03		/* End of initial setting. */
#endif
#define VGA_PERIOD_PIXEL_NUMS						(640)
#define VGA_PERIOD_LINE_NUMS						(480)

#define SVGA_PERIOD_PIXEL_NUMS						(800)
#define SVGA_PERIOD_LINE_NUMS						(600)

#define IMAGE_SENSOR_SVGA_GRAB_PIXELS			0
#define IMAGE_SENSOR_SVGA_GRAB_LINES			1

#define IMAGE_SENSOR_UXGA_GRAB_PIXELS			0
#define IMAGE_SENSOR_UXGA_GRAB_LINES			1

#define IMAGE_SENSOR_SVGA_WIDTH					(800)
#define IMAGE_SENSOR_SVGA_HEIGHT					(600)

#define IMAGE_SENSOR_UXGA_WIDTH					(1600-16)
#define IMAGE_SENSOR_UXGA_HEIGHT					(1200-12)

typedef struct
	{
		kal_uint16 init_reg;
		kal_uint16 init_val;	/* Save the register value and delay tick */
		kal_uint8 op_code;		/* 0 - Initial value, 1 - Register, 2 - Delay, 3 - End of setting. */
	} SIV121DU_initial_set_struct;


/* START GRAB PIXEL OFFSET */
#define SIV121DU_IMAGE_SENSOR_START_GRAB_X	(1)
#define SIV121DU_IMAGE_SENSOR_START_GRAB_Y	(1)

/* SENSOR PV SIZE */
#define SIV121DU_IMAGE_SENSOR_PV_WIDTH   (640 - 16)
#define SIV121DU_IMAGE_SENSOR_PV_HEIGHT  (480 - 12)


/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define SIV121DU_PERIOD_PIXEL_NUMS          (648 +144 + 1)/* Active + HST + 3;default pixel#(w/o dummy pixels) in VGA mode*/
#define SIV121DU_PERIOD_LINE_NUMS           (488 + 7 + 1)      /* Active + 9 ;default line#(w/o dummy lines) in VGA mode*/

#define SIV121DU_BLANK_REGISTER_LIMITATION   0xFFF

/*50Hz,60Hz*/
#define SIV121DU_NUM_50HZ    (50 * 2)
#define SIV121DU_NUM_60HZ    (60 * 2)
#define SIV121DU_CLK_1MHZ    (1000000)

/* FRAME RATE UNIT */
#define SIV121DU_FRAME_RATE_UNIT             10
#define SIV121DU_FPS(x)               		(SIV121DU_FRAME_RATE_UNIT * (x))

/* SENSOR READ/WRITE ID */
#define SIV121DU_WRITE_ID					0x66
#define SIV121DU_READ_ID						0x67

/* SENSOR CHIP VERSION */
#define SIV121DU_SENSOR_ID					0xDE
#define SIV121DU_SENSOR_VERSION				0x10

//export functions
UINT32 SIV121DUOpen(void);
UINT32 SIV121DUGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 SIV121DUGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 SIV121DUControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 SIV121DUFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 SIV121DUClose(void);

#endif /* __SENSOR_H */
