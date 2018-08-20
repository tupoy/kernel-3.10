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
 *   siv122du_Sensor.c
 *
 * Project:
 * --------
 *   MAUI
 *
 * Description:
 * ------------
 *   Image sensor driver function
 * 
 *
 * Author: 
 * -------
 *   Anyuan Huang (MTK70663)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Log$
 * 
 *   
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *=============================================================
 ******************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "siv122duyuv_Sensor.h"
#include "siv122duyuv_Camera_Sensor_para.h"
#include "siv122duyuv_CameraCustomized.h"


/*******************************************************************************
 * // Adapter for Winmo typedef
 ********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

#define DEBUG_SENSOR_siv122du


kal_uint32 siv122du_isp_master_clock;
MSDK_SENSOR_CONFIG_STRUCT siv122duSensorConfigData;


#define SIV122DUYUV_DEBUG

#ifdef SIV122DUYUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

typedef struct
{
  kal_uint8	    iSensorVersion;
  kal_bool		bNightMode;
  kal_uint16	iWB;
  kal_uint16	iAE;
  kal_uint16 	iEffect;
  kal_uint16 	iEV;
  kal_uint16 	iBanding;
  kal_bool		bFixFrameRate;
  kal_uint8 	iMirror;
  kal_uint16	iDummyPixel;		/* dummy pixel for user customization */
  kal_bool		bVideoMode;
  kal_uint8		iPclk;
  kal_uint16	MinFpsNormal; 
  kal_uint16	MinFpsNight; 
  /* Sensor regester backup*/
  kal_uint8		iCurrentPage;
  kal_uint8		iControl;
  kal_uint16	iHblank;			  /* dummy pixel for calculating shutter step*/
  kal_uint16	iVblank;			  /* dummy line calculated by cal_fps*/
  kal_uint8		iShutterStep;
  kal_uint8		iFrameCount;
} SIV122DUStatus;

#ifdef SIV122DU_LOAD_FROM_T_FLASH
	SIV122DU_initial_set_struct SIV122DU_Init_Reg[1000];
	WCHAR SIV122DU_set_file_name[256] = {0};
#endif


#define Sleep(ms) mdelay(ms)

/* Global Valuable */
SIV122DUStatus SIV122DUCurrentStatus;
MSDK_SENSOR_CONFIG_STRUCT SIV122DUSensorConfigData;

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 SIV122DUWriteCmosSensor(kal_uint32 addr, kal_uint32 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) ,(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 2,SIV122DU_WRITE_ID);
}

kal_uint16 SIV122DUReadCmosSensor(kal_uint32 addr)
{
	char puGetByte=0;
    char puSendCmd = (char)(addr & 0xFF);
	iReadRegI2C(&puSendCmd , 1, &puGetByte,1,SIV122DU_WRITE_ID);
    return puGetByte;
}

static void SIV122DUSetPage(kal_uint8 iPage)
{   
	
	if(SIV122DUCurrentStatus.iCurrentPage == iPage)
		return ;

	SIV122DUCurrentStatus.iCurrentPage = iPage;
    SIV122DUWriteCmosSensor(0x00,iPage);
}


      
static void SIV122DUInitialPara(void)
{
	SIV122DUCurrentStatus.bNightMode = KAL_FALSE;
	SIV122DUCurrentStatus.iWB = AWB_MODE_AUTO;
	SIV122DUCurrentStatus.iAE = AE_MODE_AUTO;
	SIV122DUCurrentStatus.iEffect = MEFFECT_OFF;
	SIV122DUCurrentStatus.iBanding = SIV122DU_NUM_50HZ;
	SIV122DUCurrentStatus.iEV = AE_EV_COMP_00;
	SIV122DUCurrentStatus.bFixFrameRate = KAL_FALSE;
	SIV122DUCurrentStatus.iMirror = IMAGE_NORMAL;
	SIV122DUCurrentStatus.iDummyPixel = 0xCD; 	  
	SIV122DUCurrentStatus.bVideoMode = KAL_FALSE;
	SIV122DUCurrentStatus.iPclk = 26;
	
	SIV122DUCurrentStatus.iCurrentPage = 0;
	SIV122DUCurrentStatus.iControl = 0x00;//0x00; 03 tlc          
	SIV122DUCurrentStatus.iHblank = 0xCD;
	SIV122DUCurrentStatus.iVblank = 0x19;
	SIV122DUCurrentStatus.iShutterStep = 0x82;
	SIV122DUCurrentStatus.iFrameCount = 0x0A;

	SIV122DUCurrentStatus.MinFpsNormal = SIV122DU_FPS(6);
	SIV122DUCurrentStatus.MinFpsNight =  SIV122DUCurrentStatus.MinFpsNormal >> 1;
}
  
static void SIV122DUInitialSetting(void)
{
    SIV122DUWriteCmosSensor(0x00, 0x00);
	SIV122DUWriteCmosSensor(0x00, 0x01);
	SIV122DUWriteCmosSensor(0x03, 0x08);
	Sleep(20);
	SIV122DUWriteCmosSensor(0x00, 0x01);
	SIV122DUWriteCmosSensor(0x03, 0x08);
	Sleep(20);
	
	SIV122DUWriteCmosSensor(0x00, 0x00);
	SIV122DUWriteCmosSensor(0x03, 0x04);
	SIV122DUWriteCmosSensor(0x10, 0x86);
	SIV122DUWriteCmosSensor(0x11, 0x74);	

	SIV122DUWriteCmosSensor(0x00, 0x01);
	SIV122DUWriteCmosSensor(0x04, 0x00);//0x00; //0x03    tlc  
	SIV122DUWriteCmosSensor(0x06, 0x04); 

	SIV122DUWriteCmosSensor(0x10, 0x11); 
	SIV122DUWriteCmosSensor(0x11, 0x25); 
	SIV122DUWriteCmosSensor(0x12, 0x21);

	SIV122DUWriteCmosSensor(0x17, 0x94); //ABS 1.74V
	SIV122DUWriteCmosSensor(0x18, 0x00);

	SIV122DUWriteCmosSensor(0x20, 0x00);
	SIV122DUWriteCmosSensor(0x21, 0xCD);
	SIV122DUWriteCmosSensor(0x22, 0x01);
	SIV122DUWriteCmosSensor(0x23, 0x19);

	SIV122DUWriteCmosSensor(0x40, 0x0F); 
	SIV122DUWriteCmosSensor(0x41, 0x90);//0x90
	SIV122DUWriteCmosSensor(0x42, 0xd2);//0xd2
	SIV122DUWriteCmosSensor(0x43, 0x00);

	// AE
	SIV122DUWriteCmosSensor(0x00, 0x02);
	SIV122DUWriteCmosSensor(0x10, 0x84);
	SIV122DUWriteCmosSensor(0x11, 0x10);		
	SIV122DUWriteCmosSensor(0x12, 0x76); // D65 target 
	SIV122DUWriteCmosSensor(0x14, 0x75); // A target
	SIV122DUWriteCmosSensor(0x34, 0x82);
	SIV122DUWriteCmosSensor(0x40, 0x40); // Max x6
	/*
	SIV122DUWriteCmosSensor(0x41, 0x28);
	SIV122DUWriteCmosSensor(0x42, 0x28);
	SIV122DUWriteCmosSensor(0x44, 0x08);
	SIV122DUWriteCmosSensor(0x45, 0x08);
	SIV122DUWriteCmosSensor(0x46, 0x15);
	SIV122DUWriteCmosSensor(0x47, 0x1c);
	SIV122DUWriteCmosSensor(0x48, 0x20);
	SIV122DUWriteCmosSensor(0x49, 0x21);
	SIV122DUWriteCmosSensor(0x4a, 0x23);
	SIV122DUWriteCmosSensor(0x4b, 0x24);
	SIV122DUWriteCmosSensor(0x4c, 0x25);
	SIV122DUWriteCmosSensor(0x4d, 0x28);
	SIV122DUWriteCmosSensor(0x4e, 0x1c);
	SIV122DUWriteCmosSensor(0x4f, 0x15);
	SIV122DUWriteCmosSensor(0x50, 0x12);
	SIV122DUWriteCmosSensor(0x51, 0x10);
	SIV122DUWriteCmosSensor(0x52, 0x0e);
	SIV122DUWriteCmosSensor(0x53, 0x0c);
	SIV122DUWriteCmosSensor(0x54, 0x0b);
	SIV122DUWriteCmosSensor(0x55, 0x0a);
	SIV122DUWriteCmosSensor(0x44, 0x08);
	SIV122DUWriteCmosSensor(0x5f, 0x01);
	SIV122DUWriteCmosSensor(0x90, 0x80);
	SIV122DUWriteCmosSensor(0x91, 0x80);
	*/

	// AWB
	SIV122DUWriteCmosSensor(0x00, 0x03);
	SIV122DUWriteCmosSensor(0x10, 0xd0);
	SIV122DUWriteCmosSensor(0x11, 0xc1);
	SIV122DUWriteCmosSensor(0x13, 0x80); //Cr target
	SIV122DUWriteCmosSensor(0x14, 0x80); //Cb target
	SIV122DUWriteCmosSensor(0x15, 0xe0); // R gain Top
	SIV122DUWriteCmosSensor(0x16, 0x7C); // R gain bottom 
	SIV122DUWriteCmosSensor(0x17, 0xe0); // B gain Top
	SIV122DUWriteCmosSensor(0x18, 0x80); // B gain bottom 0x80
	SIV122DUWriteCmosSensor(0x19, 0x8C); // Cr top value 0x90
	SIV122DUWriteCmosSensor(0x1a, 0x64); // Cr bottom value 0x70
	SIV122DUWriteCmosSensor(0x1b, 0x94); // Cb top value 0x90
	SIV122DUWriteCmosSensor(0x1c, 0x6c); // Cb bottom value 0x70
	SIV122DUWriteCmosSensor(0x1d, 0x94); // 0xa0
	SIV122DUWriteCmosSensor(0x1e, 0x6c); // 0x60
	SIV122DUWriteCmosSensor(0x20, 0xe8); // AWB luminous top value
	SIV122DUWriteCmosSensor(0x21, 0x30); // AWB luminous bottom value 0x20
	SIV122DUWriteCmosSensor(0x22, 0xb8);
	SIV122DUWriteCmosSensor(0x23, 0x10);
	SIV122DUWriteCmosSensor(0x25, 0x08);
	SIV122DUWriteCmosSensor(0x26, 0x0f);
	SIV122DUWriteCmosSensor(0x27, 0x60); // BRTSRT 
	SIV122DUWriteCmosSensor(0x28, 0x70); // BRTEND
	SIV122DUWriteCmosSensor(0x29, 0xb7); // BRTRGNBOT 
	SIV122DUWriteCmosSensor(0x2a, 0xa3); // BRTBGNTOP

	SIV122DUWriteCmosSensor(0x40, 0x01);
	SIV122DUWriteCmosSensor(0x41, 0x03);
	SIV122DUWriteCmosSensor(0x42, 0x08);
	SIV122DUWriteCmosSensor(0x43, 0x10);
	SIV122DUWriteCmosSensor(0x44, 0x13);
	SIV122DUWriteCmosSensor(0x45, 0x6a);
	SIV122DUWriteCmosSensor(0x46, 0xca);

	SIV122DUWriteCmosSensor(0x62, 0x80);
	SIV122DUWriteCmosSensor(0x63, 0x90); // R D30 to D20
	SIV122DUWriteCmosSensor(0x64, 0xd0); // B D30 to D20
	SIV122DUWriteCmosSensor(0x65, 0x98); // R D20 to D30
	SIV122DUWriteCmosSensor(0x66, 0xd0); // B D20 to D30

	// IDP
	SIV122DUWriteCmosSensor(0x00, 0x04);
	SIV122DUWriteCmosSensor(0x10, 0x7f);
	SIV122DUWriteCmosSensor(0x11, 0x1d);
	SIV122DUWriteCmosSensor(0x12, 0x0d);//2d);//fd
	SIV122DUWriteCmosSensor(0x14, 0x01);//fd
	// DPCBNR
	SIV122DUWriteCmosSensor(0x18, 0xbF);	// DPCNRCTRL
	SIV122DUWriteCmosSensor(0x19, 0x00);	// DPCTHV
	SIV122DUWriteCmosSensor(0x1A, 0x00);	// DPCTHVSLP
	SIV122DUWriteCmosSensor(0x1B, 0x00);	// DPCTHVDIFSRT
	//SIV122DUWriteCmosSensor(0x1C, 0x0f);	// DPCTHVDIFSLP
	//SIV122DUWriteCmosSensor(0x1d, 0xFF);	// DPCTHVMAX

	SIV122DUWriteCmosSensor(0x1E, 0x04);	// BNRTHV  0c
	SIV122DUWriteCmosSensor(0x1F, 0x08);	// BNRTHVSLPN 10
	SIV122DUWriteCmosSensor(0x20, 0x20);	// BNRTHVSLPD
	SIV122DUWriteCmosSensor(0x21, 0x00);	// BNRNEICNT
	SIV122DUWriteCmosSensor(0x22, 0x08);	// STRTNOR
	SIV122DUWriteCmosSensor(0x23, 0x38);	// STRTDRK
    SIV122DUWriteCmosSensor(0x24, 0x00);
	// Gamma
#if 1
	SIV122DUWriteCmosSensor(0x31, 0x03); //0x08
	SIV122DUWriteCmosSensor(0x32, 0x0B); //0x10
	SIV122DUWriteCmosSensor(0x33, 0x1E); //0x1B
	SIV122DUWriteCmosSensor(0x34, 0x46); //0x37
	SIV122DUWriteCmosSensor(0x35, 0x62); //0x4D
	SIV122DUWriteCmosSensor(0x36, 0x78); //0x60
	SIV122DUWriteCmosSensor(0x37, 0x8b); //0x72
	SIV122DUWriteCmosSensor(0x38, 0x9B); //0x82
	SIV122DUWriteCmosSensor(0x39, 0xA8); //0x91
	SIV122DUWriteCmosSensor(0x3a, 0xb6); //0xA0
	SIV122DUWriteCmosSensor(0x3b, 0xcc); //0xBA
	SIV122DUWriteCmosSensor(0x3c, 0xDF); //0xD3
	SIV122DUWriteCmosSensor(0x3d, 0xf0); //0xEA
#else
	SIV122DUWriteCmosSensor(0x31, 0x02); //0x08
	SIV122DUWriteCmosSensor(0x32, 0x09); //0x10
	SIV122DUWriteCmosSensor(0x33, 0x20); //0x1B
	SIV122DWriteCmosSensor(0x34, 0x44); //0x37
	SIV122DWriteCmosSensor(0x35, 0x62); //0x4D
	SIV122DWriteCmosSensor(0x36, 0x7c); //0x60
	SIV122DWriteCmosSensor(0x37, 0x8f); //0x72
	SIV122DWriteCmosSensor(0x38, 0x9f); //0x82
	SIV122DWriteCmosSensor(0x39, 0xae); //0x91
	SIV122DWriteCmosSensor(0x3a, 0xba); //0xA0
	SIV122DWriteCmosSensor(0x3b, 0xcf); //0xBA
	SIV122DWriteCmosSensor(0x3c, 0xe1); //0xD3
	SIV122DWriteCmosSensor(0x3d, 0xf0); //0xEA
#endif

	// Shading Register Setting 				 
	SIV122DUWriteCmosSensor(0x40, 0x06);						   
	SIV122DUWriteCmosSensor(0x41, 0x44);						   
	SIV122DUWriteCmosSensor(0x42, 0x43);						   
	SIV122DUWriteCmosSensor(0x43, 0x20);						   
	SIV122DUWriteCmosSensor(0x44, 0x11); // left R gain[7:4], right R gain[3:0] 						
	SIV122DUWriteCmosSensor(0x45, 0x11); // top R gain[7:4], bottom R gain[3:0] 						 
	SIV122DUWriteCmosSensor(0x46, 0x00); // left G gain[7:4], right G gain[3:0] 	  
	SIV122DUWriteCmosSensor(0x47, 0x00); // top G gain[7:4], bottom G gain[3:0] 	  
	SIV122DUWriteCmosSensor(0x48, 0x00); // left B gain[7:4], right B gain[3:0] 	  
	SIV122DUWriteCmosSensor(0x49, 0x00); // top B gain[7:4], bottom B gain[3:0] 	 
	SIV122DUWriteCmosSensor(0x4a, 0x04); // X-axis center high[3:2], Y-axis center high[1:0]	 
	SIV122DUWriteCmosSensor(0x4b, 0x48); // X-axis center low[7:0]						
	SIV122DUWriteCmosSensor(0x4c, 0xe8); // Y-axis center low[7:0]					   
	SIV122DUWriteCmosSensor(0x4d, 0x80); // Shading Center Gain


	// Interpolation
	SIV122DUWriteCmosSensor(0x60,  0x7F);
	SIV122DUWriteCmosSensor(0x61,  0x08);	// INTCTRL outdoor
#if 0
	// Color matrix (D65) - Daylight
	SIV122DUWriteCmosSensor(0x71, 0x3B);
	SIV122DUWriteCmosSensor(0x72, 0xC7);
	SIV122DUWriteCmosSensor(0x73, 0xFe);
	SIV122DUWriteCmosSensor(0x74, 0x10);
	SIV122DUWriteCmosSensor(0x75, 0x28);
	SIV122DUWriteCmosSensor(0x76, 0x08);
	SIV122DUWriteCmosSensor(0x77, 0xec);
	SIV122DUWriteCmosSensor(0x78, 0xCd);
	SIV122DUWriteCmosSensor(0x79, 0x47);

	// Color matrix (D20) - A
	SIV122DUWriteCmosSensor(0x83, 0x38); //0x3c 	
	SIV122DUWriteCmosSensor(0x84, 0xd1); //0xc6 	
	SIV122DUWriteCmosSensor(0x85, 0xf7); //0xff   
	SIV122DUWriteCmosSensor(0x86, 0x12); //0x12    
	SIV122DUWriteCmosSensor(0x87, 0x25); //0x24 	
	SIV122DUWriteCmosSensor(0x88, 0x09); //0x0a 	
	SIV122DUWriteCmosSensor(0x89, 0xed); //0xed   
	SIV122DUWriteCmosSensor(0x8a, 0xbb); //0xc2 	
	SIV122DUWriteCmosSensor(0x8b, 0x58); //0x51
#else
	// Color matrix (D65) - Daylight
	SIV122DUWriteCmosSensor(0x71, 0x39);
	SIV122DUWriteCmosSensor(0x72, 0xC8);
	SIV122DUWriteCmosSensor(0x73, 0xFf);
	SIV122DUWriteCmosSensor(0x74, 0x13);
	SIV122DUWriteCmosSensor(0x75, 0x25);
	SIV122DUWriteCmosSensor(0x76, 0x08);
	SIV122DUWriteCmosSensor(0x77, 0xf8);
	SIV122DUWriteCmosSensor(0x78, 0xC0);
	SIV122DUWriteCmosSensor(0x79, 0x48);

	// Color matrix (D20) - A
	SIV122DUWriteCmosSensor(0x83, 0x34); //0x3c 	
	SIV122DUWriteCmosSensor(0x84, 0xd4); //0xc6 	
	SIV122DUWriteCmosSensor(0x85, 0xf8); //0xff   
	SIV122DUWriteCmosSensor(0x86, 0x12); //0x12    
	SIV122DUWriteCmosSensor(0x87, 0x25); //0x24 	
	SIV122DUWriteCmosSensor(0x88, 0x09); //0x0a 	
	SIV122DUWriteCmosSensor(0x89, 0xf0); //0xed   
	SIV122DUWriteCmosSensor(0x8a, 0xbe); //0xc2 	
	SIV122DUWriteCmosSensor(0x8b, 0x54); //0x51
#endif
	SIV122DUWriteCmosSensor(0x8c, 0x10); //CMA select	  

	//G Edge
	SIV122DUWriteCmosSensor(0x90, 0x35); //Upper gain
	SIV122DUWriteCmosSensor(0x91, 0x48); //down gain
	SIV122DUWriteCmosSensor(0x92, 0x33); //[7:4] upper coring [3:0] down coring
	SIV122DUWriteCmosSensor(0x9a, 0x40);
	SIV122DUWriteCmosSensor(0x9b, 0x40);
	SIV122DUWriteCmosSensor(0x9c, 0x38); //edge suppress start
	SIV122DUWriteCmosSensor(0x9d, 0x30); //edge suppress slope

	SIV122DUWriteCmosSensor(0x9f, 0x26);
	SIV122DUWriteCmosSensor(0xa0, 0x11);

	SIV122DUWriteCmosSensor(0xa8, 0x11);
	SIV122DUWriteCmosSensor(0xa9, 0x12);
	SIV122DUWriteCmosSensor(0xaa, 0x12);

	SIV122DUWriteCmosSensor(0xb9, 0x28); // nightmode 38 at gain 0x48 5fps
	SIV122DUWriteCmosSensor(0xba, 0x41); // nightmode 80 at gain 0x48 5fps


	SIV122DUWriteCmosSensor(0xbf, 0x20);
	SIV122DUWriteCmosSensor(0xde, 0x80);
	/*
	SIV122DUWriteCmosSensor(0xc0, 0x24);	 
	SIV122DUWriteCmosSensor(0xc1, 0x00);	 
	SIV122DUWriteCmosSensor(0xc2, 0x80);	
	SIV122DUWriteCmosSensor(0xc3, 0x00);	 
	SIV122DUWriteCmosSensor(0xc4, 0xe0);	 
	SIV122DUWriteCmosSensor(0xcb,0x00);
	SIV122DUWriteCmosSensor(0xde, 0x80);
	*/

	SIV122DUWriteCmosSensor(0xe5, 0x15); //MEMSPDA
	SIV122DUWriteCmosSensor(0xe6, 0x02); //MEMSPDB
	SIV122DUWriteCmosSensor(0xe7, 0x04); //MEMSPDC
 

	//Sensor On  
	SIV122DUWriteCmosSensor(0x00, 0x01);
	SIV122DUWriteCmosSensor(0x03, 0x01); // SNR Enable
}   /* SIV122DUInitialSetting */   /* SIV122DUInitialSetting */


/*************************************************************************
* FUNCTION
*    SIV122DHalfAdjust
*
* DESCRIPTION
*    This function dividend / divisor and use round-up.
*
* PARAMETERS
*    dividend
*    divisor
*
* RETURNS
*    [dividend / divisor]
*
* LOCAL AFFECTED
*
*************************************************************************/
__inline static kal_uint32 SIV122DUHalfAdjust(kal_uint32 dividend, kal_uint32 divisor)
{
  return (dividend * 2 + divisor) / (divisor * 2); /* that is [dividend / divisor + 0.5]*/
}

/*************************************************************************
* FUNCTION
*   SIV122DUSetShutterStep
*
* DESCRIPTION
*   This function is to calculate & set shutter step register .
*
*************************************************************************/
static void SIV122DUSetShutterStep(void)
{       
    const kal_uint8 banding = SIV122DUCurrentStatus.iBanding == AE_FLICKER_MODE_50HZ ? SIV122DU_NUM_50HZ : SIV122DU_NUM_60HZ;
    const kal_uint16 shutter_step = SIV122DUHalfAdjust(SIV122DUCurrentStatus.iPclk * SIV122DU_CLK_1MHZ / 2, (SIV122DUCurrentStatus.iHblank + SIV122DU_PERIOD_PIXEL_NUMS) * banding);

	if(SIV122DUCurrentStatus.iShutterStep == shutter_step)
		return ;

	SIV122DUCurrentStatus.iShutterStep = shutter_step;
    ASSERT(shutter_step <= 0xFF);    
    /* Block 1:0x34  shutter step*/
    SIV122DUSetPage(2);
    SIV122DUWriteCmosSensor(0x34,shutter_step);
	
    SENSORDB("Set Shutter Step:%x\n",shutter_step);
}/* SIV122DUSetShutterStep */

/*************************************************************************
* FUNCTION
*   SIV122DUSetFrameCount
*
* DESCRIPTION
*   This function is to set frame count register .
*
*************************************************************************/
static void SIV122DUSetFrameCount(void)
{    
    kal_uint16 Frame_Count,min_fps = 60;
    kal_uint8 banding = SIV122DUCurrentStatus.iBanding == AE_FLICKER_MODE_50HZ ? SIV122DU_NUM_50HZ : SIV122DU_NUM_60HZ;

	min_fps = SIV122DUCurrentStatus.bNightMode ? SIV122DUCurrentStatus.MinFpsNight : SIV122DUCurrentStatus.MinFpsNormal;
    Frame_Count = banding * SIV122DU_FRAME_RATE_UNIT / min_fps;

	if(SIV122DUCurrentStatus.iFrameCount == Frame_Count)
		return ;

	SIV122DUCurrentStatus.iFrameCount = Frame_Count;
	
    SENSORDB("min_fps:%d,Frame_Count:%x\n",min_fps/SIV122DU_FRAME_RATE_UNIT,Frame_Count);
    /*Block 01: 0x11  Max shutter step,for Min frame rate */
    SIV122DUSetPage(2);
    SIV122DUWriteCmosSensor(0x11,Frame_Count&0xFF);    
}/* SIV122DUSetFrameCount */

/*************************************************************************
* FUNCTION
*   SIV122DUConfigBlank
*
* DESCRIPTION
*   This function is to set Blank size for Preview mode .
*
* PARAMETERS
*   iBlank: target HBlank size
*      iHz: banding frequency
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void SIV122DUConfigBlank(kal_uint16 hblank,kal_uint16 vblank)
{    
    SENSORDB("hblank:%x,vblank:%x\n",hblank,vblank);
   /********************************************    
    *   Register :0x20 - 0x22
    *  Block 00
    *  0x20  [7:4]:HBANK[9:8]; 0x20  [3:0]:VBANK[9:8]
    *  0x21 HBANK[7:0]
    *  0x23 VBANK[7:0]  
    ********************************************/
	if((SIV122DUCurrentStatus.iHblank == hblank) && (SIV122DUCurrentStatus.iVblank == vblank) )
	   return ;

	SIV122DUCurrentStatus.iHblank = hblank;
	SIV122DUCurrentStatus.iVblank = vblank;
	ASSERT((hblank <= SIV122DU_BLANK_REGISTER_LIMITATION) && (vblank <= SIV122DU_BLANK_REGISTER_LIMITATION)); //这里有问题导致手机重启 ，没有加括号，导致优先级出错
	SIV122DUSetPage(1);
	SIV122DUWriteCmosSensor(0x20,((hblank>>4)&0xF0)|((vblank>>8)&0x0F));
	SIV122DUWriteCmosSensor(0x21,hblank & 0xFF);
	SIV122DUWriteCmosSensor(0x23,vblank & 0xFF);
	SIV122DUSetShutterStep();
}   /* SIV122DUConfigBlank */

/*************************************************************************
* FUNCTION
*    SIV122DUCalFps
*
* DESCRIPTION
*    This function calculate & set frame rate and fix frame rate when video mode
*    MUST BE INVOKED AFTER SIM120C_preview() !!!
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void SIV122DUCalFps(void)
{
    kal_uint16 Line_length,Dummy_Line,Dummy_Pixel;
  kal_uint16 max_fps = 250;

    Line_length = SIV122DUCurrentStatus.iDummyPixel + SIV122DU_PERIOD_PIXEL_NUMS; 
	
	if (SIV122DUCurrentStatus.bVideoMode == KAL_TRUE)
	{		
		max_fps = SIV122DUCurrentStatus.bNightMode ? SIV122DUCurrentStatus.MinFpsNight: SIV122DUCurrentStatus.MinFpsNormal;
	}
	else
	{		
		max_fps = SIV122DU_FPS(25);
	}
	
    Dummy_Line = SIV122DUCurrentStatus.iPclk * SIV122DU_CLK_1MHZ * SIV122DU_FRAME_RATE_UNIT / (2 * Line_length * max_fps) - SIV122DU_PERIOD_LINE_NUMS; 
    if(Dummy_Line > SIV122DU_BLANK_REGISTER_LIMITATION)
    {
        Dummy_Line = SIV122DU_BLANK_REGISTER_LIMITATION;
        Line_length = SIV122DUCurrentStatus.iPclk * SIV122DU_CLK_1MHZ * SIV122DU_FRAME_RATE_UNIT / (2 * (Dummy_Line + SIV122DU_PERIOD_LINE_NUMS) * max_fps);
    }
    Dummy_Pixel = Line_length -  SIV122DU_PERIOD_PIXEL_NUMS;
	
    SENSORDB("max_fps:%d\n",max_fps/SIV122DU_FRAME_RATE_UNIT);
    SENSORDB("Dummy Pixel:%x,Hblank:%x,Vblank:%x\n",SIV122DUCurrentStatus.iDummyPixel,Dummy_Pixel,Dummy_Line);
    SIV122DUConfigBlank((Dummy_Pixel > 0) ? Dummy_Pixel : 0, (Dummy_Line > 0) ? Dummy_Line : 0);
    SIV122DUSetShutterStep();
}


/*************************************************************************
* FUNCTION
*   SIV122DUFixFrameRate
*
* DESCRIPTION
*   This function fix the frame rate of image sensor.
*
*************************************************************************/
static void SIV122DUFixFrameRate(kal_bool bEnable)
{
	if(SIV122DUCurrentStatus.bFixFrameRate == bEnable)
		return ;

	SIV122DUCurrentStatus.bFixFrameRate = bEnable;
    if(bEnable == KAL_TRUE)
    {   //fix frame rate
        SIV122DUCurrentStatus.iControl |= 0xC0;
    }
    else
    {        
        SIV122DUCurrentStatus.iControl &= 0x3F;
    }
	SIV122DUSetPage(1);
	SIV122DUWriteCmosSensor(0x04,SIV122DUCurrentStatus.iControl);
}   /* SIV122DUFixFrameRate */

/*************************************************************************
* FUNCTION
*   SIV122DUHVmirror
*
* DESCRIPTION
*   This function config the HVmirror of image sensor.
*
*************************************************************************/
static void SIV122DUHVmirror(kal_uint8 HVmirrorType)
{   
	if(SIV122DUCurrentStatus.iMirror == HVmirrorType)
		return ;

	SIV122DUCurrentStatus.iMirror = HVmirrorType;
    SIV122DUCurrentStatus.iControl = SIV122DUCurrentStatus.iControl & 0xFC;  
    switch (HVmirrorType) 
    {
        case IMAGE_H_MIRROR:
            SIV122DUCurrentStatus.iControl |= 0x01;
            break;
            
        case IMAGE_V_MIRROR:
            SIV122DUCurrentStatus.iControl |= 0x02;
            break;
                
        case IMAGE_HV_MIRROR:
            SIV122DUCurrentStatus.iControl |= 0x03;
            break;
		case IMAGE_NORMAL:
        default:
            SIV122DUCurrentStatus.iControl |= 0x00;
    }    
    SIV122DUSetPage(1);
    SIV122DUWriteCmosSensor(0x04,SIV122DUCurrentStatus.iControl);
}   /* SIV122DUHVmirror */

/*************************************************************************
* FUNCTION
*	SIV122DUNightMode
*
* DESCRIPTION
*	This function night mode of SIV122DU.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void SIV122DUNightMode(kal_bool enable)
{
    SENSORDB("NightMode %d",enable);

    SIV122DUCurrentStatus.bNightMode = enable;

  if ( SIV122DUCurrentStatus.bVideoMode == KAL_TRUE)// camera mode
    return ;
  
		SIV122DUSetPage(2);
		//SIV122DUWriteCmosSensor(0x40,0x64);//Max Analog Gain Value @ Shutter step = Max Shutter step  0x7D
        if (SIV122DUCurrentStatus.bNightMode == KAL_TRUE)
        {   /* camera night mode */
          //  PK_DBG("camera night mode\n");
           // SIV122DUSetPage(1);
           SIV122DUWriteCmosSensor(0x40,0x50); //Max Analog Gain Value @ Shutter step = Max Shutter step  0x7D
            //SIV122DUSetPage(3);
            //SIV122DUWriteCmosSensor(0xAB,0x98); //Brightness Control 0x11
            //SIV122DUWriteCmosSensor(0xB9,0x18); //Color Suppression Change Start State  0x18           
            //SIV122DUWriteCmosSensor(0xBA,0x20); //Slope
        }
        else
        {   /* camera normal mode */
          //  PK_DBG("camera normal mode\n");
            //SIV122DUSetPage(1);
            SIV122DUWriteCmosSensor(0x40,0x40);// 0x7F
            //SIV122DUSetPage(3);
            //SIV122DUWriteCmosSensor(0xAB,0x98); //0x04 
            //SIV122DUWriteCmosSensor(0xB9,0x18);            
            //SIV122DUWriteCmosSensor(0xBA,0x20); //Slope
        }
		SIV122DUSetFrameCount();    
}  /* SIV122DUNightMode */

#ifdef SIV122DU_LOAD_FROM_T_FLASH
/*************************************************************************
* FUNCTION
*	SIV122DU_Initialize_from_T_Flash
*
* DESCRIPTION
*	Read the initialize setting from t-flash or user disk to speed up image quality tuning.
*
* PARAMETERS
*	None
*
* RETURNS
*	kal_uint8 - 0 : Load setting fail, 1 : Load setting successfully.
*
*************************************************************************/
static kal_uint8 SIV122DU_Initialize_from_T_Flash()
{
	FS_HANDLE fp = -1;				/* Default, no file opened. */
	kal_uint8 *data_buff = NULL;
	kal_uint8 *curr_ptr = NULL;
	kal_uint32 file_size = 0;
	kal_uint32 bytes_read = 0;
	kal_uint32 i = 0, j = 0;
	kal_uint8 func_ind[3] = {0};	/* REG or DLY */

	kal_mem_cpy(SIV122DU_set_file_name, L"C:\\SIV122DU_Initialize_Setting.Bin", sizeof(L"C:\\SIV122DU_Initialize_Setting.Bin"));

	/* Search the setting file in all of the user disk. */
	curr_ptr = (kal_uint8 *)SIV122DU_set_file_name;
	
	while (fp < 0)
	{
		if ((*curr_ptr >= 'c' && *curr_ptr <= 'z') || (*curr_ptr >= 'C' && *curr_ptr <= 'Z'))
		{
			fp = FS_Open(SIV122DU_set_file_name, FS_READ_ONLY);
			if (fp >= 0)
			{
				break;			/* Find the setting file. */
			}

			*curr_ptr = *curr_ptr + 1;
		}
		else
		{
			break ;
		}
	}
	

	if (fp < 0)		/* Error handle */
	{
	#ifdef __SIV122DU_DEBUG_TRACE__
		kal_wap_trace(MOD_ENG, TRACE_INFO, "!!! Warning, Can't find the initial setting file!!!");
	#endif
	
		return 0;
	}

	FS_GetFileSize(fp, &file_size);
	if (file_size < 20)
	{
	#ifdef __SIV122DU_DEBUG_TRACE__
		kal_wap_trace(MOD_ENG, TRACE_INFO, "!!! Warning, Invalid setting file!!!");
	#endif
	
		return 0;			/* Invalid setting file. */
	}

	data_buff = med_alloc_ext_mem(file_size);
	if (data_buff == NULL)
	{
	#ifdef __SIV122DU_DEBUG_TRACE__
		kal_wap_trace(MOD_ENG, TRACE_INFO, "!!! Warning, Memory not enoughed...");
	#endif
	
		return 0;				/* Memory not enough */
	}
	FS_Read(fp, data_buff, file_size, &bytes_read);

	/* Start parse the setting witch read from t-flash. */
	curr_ptr = data_buff;
	while (curr_ptr < (data_buff + file_size))
	{
		while ((*curr_ptr == ' ') || (*curr_ptr == '\t'))/* Skip the Space & TAB */
			curr_ptr++;				

		if (((*curr_ptr) == '/') && ((*(curr_ptr + 1)) == '*'))
		{
			while (!(((*curr_ptr) == '*') && ((*(curr_ptr + 1)) == '/')))
			{
				curr_ptr++;		/* Skip block comment code. */
			}

			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */
			
			continue ;
		}
		
		if (((*curr_ptr) == '/') || ((*curr_ptr) == '{') || ((*curr_ptr) == '}'))		/* Comment line, skip it. */
		{
			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}
		/* This just content one enter line. */
		if (((*curr_ptr) == 0x0D) && ((*(curr_ptr + 1)) == 0x0A))
		{
			curr_ptr += 2;
			continue ;
		}

		kal_mem_cpy(func_ind, curr_ptr, 3);
		curr_ptr += 4;					/* Skip "REG(" or "DLY(" */
		if (strcmp((const char *)func_ind, "REG") == 0)		/* REG */
		{
			SIV122DU_Init_Reg[i].op_code = SIV122DU_OP_CODE_REG;
			
			SIV122DU_Init_Reg[i].init_reg = strtol((const char *)curr_ptr, NULL, 16);
			curr_ptr += SIV122DU_REG_SKIP;	/* Skip "0x0000, " */
			
			SIV122DU_Init_Reg[i].init_val = strtol((const char *)curr_ptr, NULL, 16);
			curr_ptr += SIV122DU_VAL_SKIP;	/* Skip "0x0000);" */
		}
		else									/* DLY */
		{
			/* Need add delay for this setting. */
			SIV122DU_Init_Reg[i].op_code = SIV122DU_OP_CODE_DLY;
			
			SIV122DU_Init_Reg[i].init_reg = 0xFF;
			SIV122DU_Init_Reg[i].init_val = strtol((const char *)curr_ptr, NULL, 10);	/* Get the delay ticks, the delay should less then 50 */
		}
		i++;
		

		/* Skip to next line directly. */
		while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
		{
			curr_ptr++;
		}
		curr_ptr += 2;
	}

	/* (0xFFFF, 0xFFFF) means the end of initial setting. */
	SIV122DU_Init_Reg[i].op_code = SIV122DU_OP_CODE_END;
	SIV122DU_Init_Reg[i].init_reg = 0xFF;
	SIV122DU_Init_Reg[i].init_val = 0xFF;
	i++;
	
#ifdef __SIV122DU_DEBUG_TRACE__
	kal_wap_trace(MOD_ENG, TRACE_INFO, "%d register read...", i-1);
#endif

	med_free_ext_mem((void **)&data_buff);
	FS_Close(fp);

#ifdef __SIV122DU_DEBUG_TRACE__
	kal_wap_trace(MOD_ENG, TRACE_INFO, "Start apply initialize setting.");
#endif
	/* Start apply the initial setting to sensor. */
	for (j=0; j<i; j++)
	{
		if (SIV122DU_Init_Reg[j].op_code == SIV122DU_OP_CODE_END)	/* End of the setting. */
		{
			break ;
		}
		else if (SIV122DU_Init_Reg[j].op_code == SIV122DU_OP_CODE_DLY)
		{
			kal_sleep_task(SIV122DU_Init_Reg[j].init_val);		/* Delay */
		}
		else if (SIV122DU_Init_Reg[j].op_code == SIV122DU_OP_CODE_REG)
		{
			SIV122DUWriteCmosSensor(SIV122DU_Init_Reg[j].init_reg, SIV122DU_Init_Reg[j].init_val);
		}
		else
		{
			ASSERT(0);
		}
	}

#ifdef __SIV122DU_DEBUG_TRACE__
	kal_wap_trace(MOD_ENG, TRACE_INFO, "%d register applied...", j);
#endif

	return 1;	

}
#endif


/*************************************************************************
* FUNCTION
*	SIV122DUOpen
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SIV122DUOpen(void)
{

        kal_uint16 sensor_id=0;
        int i;
        do
        {
        	// check if sensor ID correct
        for(i = 0; i < 3; i++)
	{
	        SIV122DUWriteCmosSensor(0x00,0x00);
		sensor_id = SIV122DUReadCmosSensor(0x01);
		SENSORDB("SIV122DU Sensor Read ID %x\n",sensor_id);
	        if (sensor_id == 0xDE)
		{
                    sensor_id = SIV122DU_SENSOR_ID ;
                    break;
	        }
        }

        mdelay(50);

        }while(0);
	SENSORDB("SIV122DUGetSensorID\n");	
	//	Read sensor ID to adjust I2C is OK?

	if (sensor_id != SIV122DU_SENSOR_ID) 
	{
	  return ERROR_SENSOR_CONNECT_FAIL;
	}

	//  Read sensor ID to adjust I2C is OK?

	SIV122DUCurrentStatus.iSensorVersion  = SIV122DUReadCmosSensor(0x02);	
	//PK_DBG("IV120D Sensor Version %x\n",SIV122DUCurrentStatus.iSensorVersion);
	//PK_DBG("IV120D Sensor Version %x\n",SIV122DUCurrentStatus.iSensorVersion);
	//SIV122DU_set_isp_driving_current(3);
	SIV122DUInitialPara();

    
#ifdef SIV122DU_LOAD_FROM_T_FLASH
		if (SIV122DU_Initialize_from_T_Flash())		/* For debug use. */
		{
			/* If load from t-flash success, then do nothing. */
		}
		else	/* Failed, load from the image load. */
#endif
	SIV122DUInitialSetting();
	mdelay(10);
    SIV122DUCalFps();    

	return ERROR_NONE;
}	/* SIV122DUOpen() */

                
/*************************************************************************
* FUNCTION
*   SIV122DUGetSensorID
*
* DESCRIPTION
*   This function get the sensor ID 
*
* PARAMETERS
*   *sensorID : return the sensor ID 
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SIV122DUGetSensorID(UINT32 *sensorID)   
{
	kal_uint16 sensor_id=0;
    int i;
    do
    {
        	// check if sensor ID correct
        for(i = 0; i < 3; i++)
	{
	        SIV122DUWriteCmosSensor(0x00,0x00);
	        sensor_id = SIV122DUReadCmosSensor(0x01);
		SENSORDB("SIV122DU Sensor Read ID %x\n",sensor_id);
		   printk("SIV122DU Sensor id = %x\n", sensor_id);
	        if (sensor_id == 0xDE)
		{
                    sensor_id = SIV122DU_SENSOR_ID ;
	             	break;
	        }
        }
        mdelay(50);
    }while(0);
	SENSORDB("SIV122DUGetSensorID\n");	
	//	Read sensor ID to adjust I2C is OK?

	if (sensor_id != SIV122DU_SENSOR_ID) 
	{
	  return ERROR_SENSOR_CONNECT_FAIL;
	}
	*sensorID = sensor_id;
	return ERROR_NONE;
}



/*************************************************************************
* FUNCTION
*	SIV122DUClose
*
* DESCRIPTION
*	This function is to turn off sensor module power.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SIV122DUClose(void)
{
//	CISModulePowerOn(FALSE);
	SIV122DUWriteCmosSensor(0x00, 0x01);
	SIV122DUWriteCmosSensor(0x03, 0x08);

	return ERROR_NONE;
}	/* SIV122DUClose() */

/*************************************************************************
* FUNCTION
*	SIV122DUPreview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SIV122DUPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	SENSORDB("SIV122DUPreview\r\n");

	SENSORDB("SensorOperationMode =%d\r\n",sensor_config_data->SensorOperationMode);
    /* ==Camera Preview, MT6516 use 26MHz PCLK, 30fps == */
	SENSORDB("Camera preview\r\n");
    //4  <1> preview of capture PICTURE
	//SIV122DUFixFrameRate(KAL_FALSE);
	SIV122DUCurrentStatus.MinFpsNormal = SIV122DU_FPS(6);
	SIV122DUCurrentStatus.MinFpsNight =  SIV122DUCurrentStatus.MinFpsNormal >> 1;	
	SIV122DUCurrentStatus.bVideoMode =  KAL_FALSE;
	 
    //4 <2> set mirror and flip
    //SIV122DUHVmirror(sensor_config_data->SensorImageMirror);
	//SIV122DUHVmirror(3);//--->
    //4 <3> set dummy pixel, dummy line will calculate from frame rate
	/* SIV122DUCurrentStatus.iDummyPixel = 0x1d; */	  
	
	// copy sensor_config_data
	memcpy(&SIV122DUSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
  	return ERROR_NONE;
}	/* SIV122DUPreview() */

UINT32 SIV122DUGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	//pSensorResolution->SensorFullWidth = SIV122DU_IMAGE_SENSOR_PV_WIDTH+2; //add workaround for VGA sensor
	pSensorResolution->SensorFullWidth = SIV122DU_IMAGE_SENSOR_PV_WIDTH; //+2; //add workaround for VGA sensor
	pSensorResolution->SensorFullHeight = SIV122DU_IMAGE_SENSOR_PV_HEIGHT;
	pSensorResolution->SensorPreviewWidth = SIV122DU_IMAGE_SENSOR_PV_WIDTH;
	pSensorResolution->SensorPreviewHeight = SIV122DU_IMAGE_SENSOR_PV_HEIGHT;
	pSensorResolution->SensorVideoWidth = SIV122DU_IMAGE_SENSOR_PV_WIDTH;
	pSensorResolution->SensorVideoHeight = SIV122DU_IMAGE_SENSOR_PV_HEIGHT;
	return ERROR_NONE;
}	/* SIV122DUGetResolution() */

UINT32 SIV122DUGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX = SIV122DU_IMAGE_SENSOR_PV_WIDTH;
	pSensorInfo->SensorPreviewResolutionY = SIV122DU_IMAGE_SENSOR_PV_HEIGHT;
	pSensorInfo->SensorFullResolutionX = SIV122DU_IMAGE_SENSOR_PV_WIDTH;
	pSensorInfo->SensorFullResolutionY = SIV122DU_IMAGE_SENSOR_PV_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=25;
	pSensorInfo->SensorStillCaptureFrameRate=25;
	pSensorInfo->SensorWebCamCaptureFrameRate=25;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YVYU;//SENSOR_OUTPUT_FORMAT_YVYU;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorInterruptDelayLines = 1;
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;
	pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;
    pSensorInfo->CaptureDelayFrame = 3; 
    pSensorInfo->PreviewDelayFrame = 8; 
    pSensorInfo->VideoDelayFrame = 8; 

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		//case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		default:			
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX = 1; 
			pSensorInfo->SensorGrabStartY = 1;		   
			break;
	}
	memcpy(pSensorConfigData, &SIV122DUSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* SIV122DUGetInfo() */

UINT32 SIV122DUYUVSetVideoMode(UINT16 u2FrameRate)
{
    SENSORDB("SetVideoMode %d\n",u2FrameRate);

	//SIV122DUFixFrameRate(KAL_TRUE);  
	SIV122DUCurrentStatus.bVideoMode = KAL_TRUE;
	//SIV122DUCurrentStatus.MinFpsNormal = SIV122DU_FPS(25);
	//SIV122DUCurrentStatus.MinFpsNight =  SIV122DUCurrentStatus.MinFpsNormal >> 1; 
    if (u2FrameRate == 25)
    {
		SIV122DUCurrentStatus.bNightMode = KAL_FALSE;
    }
    else if (u2FrameRate == 10)       
    {
		SIV122DUCurrentStatus.bNightMode = KAL_TRUE;
    }
    else 
    {
        printk("Wrong frame rate setting \n");
		//return FALSE;
		SIV122DUCurrentStatus.bNightMode = KAL_FALSE;
		
    }   
SIV122DUCurrentStatus.MinFpsNormal = SIV122DU_FPS(15);
	
        SIV122DUSetPage(1);
	SIV122DUWriteCmosSensor(0x20, 0x50);//00);
	SIV122DUWriteCmosSensor(0x21, 0x5b);//CD);
	SIV122DUWriteCmosSensor(0x22, 0x01);
	SIV122DUWriteCmosSensor(0x23, 0x69);//19);
	
	SIV122DUSetPage(2);
	SIV122DUWriteCmosSensor(0x11,0x0A);
	SIV122DUWriteCmosSensor(0x34,0x3c);

  SIV122DUWriteCmosSensor(0x40,0x40);//Max Analog Gain Value @ Shutter step = Max Shutter step  0x7D
	SIV122DUCalFps();	  
    SIV122DUSetFrameCount();    

	        SIV122DUSetPage(1);
	SIV122DUWriteCmosSensor(0x20, 0x50);//00);
	SIV122DUWriteCmosSensor(0x21, 0x5b);//CD);
	SIV122DUWriteCmosSensor(0x22, 0x01);
	SIV122DUWriteCmosSensor(0x23, 0x69);//19);
	
	SIV122DUSetPage(2);
	SIV122DUWriteCmosSensor(0x11,0x0A);
	SIV122DUWriteCmosSensor(0x34,0x3c);
    return TRUE;
}

UINT32 SIV122DUControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	SENSORDB("SIV122DUControl()  ScenarioId =%d\r\n",ScenarioId);
// both preview and capture ScenarioId=3;
printk("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz:mode ==%d\n",ScenarioId);
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			SIV122DUPreview(pImageWindow, pSensorConfigData);
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			SIV122DUPreview(pImageWindow, pSensorConfigData);
			SIV122DUYUVSetVideoMode(15);
			break;
		//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		//case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		default:
			SIV122DUPreview(pImageWindow, pSensorConfigData);
			//PK_DBG("pImageWindow ,pSensorConfigData\r\n",pImageWindow,pSensorConfigData);
			//PK_DBG("VControl\r\n" );
			break;
	}
	return TRUE;
}	/* SIV122DUControl() */

BOOL SIV122DUSetParamWB(UINT16 para)
{
    SENSORDB("WB %d\n",para);
	if(SIV122DUCurrentStatus.iWB== para)
		return ;

	SIV122DUCurrentStatus.iWB = para;
    SIV122DUSetPage(3);
    switch (para)
    {
        case AWB_MODE_OFF:
            SIV122DUWriteCmosSensor(0x10, 0x00);
            break;
			
        case AWB_MODE_AUTO:
            SIV122DUWriteCmosSensor(0x10, 0xD0);
            break;

        case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
	SIV122DUWriteCmosSensor(0x10, 0xD5); 
	SIV122DUWriteCmosSensor(0x72, 0xb4);
	SIV122DUWriteCmosSensor(0x73, 0x74);
            break;

        case AWB_MODE_DAYLIGHT: //sunny
	SIV122DUWriteCmosSensor(0x10, 0xD5); 
	SIV122DUWriteCmosSensor(0x72, 0xD8);
	SIV122DUWriteCmosSensor(0x73, 0x90);
            break;

        case AWB_MODE_INCANDESCENT: //office
	SIV122DUWriteCmosSensor(0x10, 0xD5); 
	SIV122DUWriteCmosSensor(0x72, 0x80);
	SIV122DUWriteCmosSensor(0x73, 0xE0);
            break;

    case AWB_MODE_TUNGSTEN: //home
	SIV122DUWriteCmosSensor(0x10, 0xD5); 
	SIV122DUWriteCmosSensor(0x72, 0xb8);
	SIV122DUWriteCmosSensor(0x73, 0xCC);
            break;
			
        case AWB_MODE_FLUORESCENT:
	SIV122DUWriteCmosSensor(0x10, 0xD5); 
	SIV122DUWriteCmosSensor(0x72, 0x78);
	SIV122DUWriteCmosSensor(0x73, 0xA0);
            break;

        default:
            return FALSE;
    }

    return TRUE;
} /* SIV122DUSetParamWB */

BOOL SIV122DUSetParamAE(UINT16 para)
{
    SENSORDB("AE %d\n",para);
	if(SIV122DUCurrentStatus.iAE== para)
		return ;

	SIV122DUCurrentStatus.iAE = para;
	SIV122DUSetPage(2);
	if(KAL_TRUE == para)
		SIV122DUWriteCmosSensor(0x10,0x80);
	else
		SIV122DUWriteCmosSensor(0x10,0x00);

    return TRUE;
} /* VSetParamAE */

BOOL SIV122DUSetParamEffect(UINT16 para)
{
    SENSORDB("Effect %d\n",para);
	if(SIV122DUCurrentStatus.iEffect== para)
		return ;

	SIV122DUCurrentStatus.iEffect = para;
    SIV122DUSetPage(4);
    switch (para)
    {
        case MEFFECT_OFF:
            SIV122DUWriteCmosSensor(0xB6, 0x00);
            break;

        case MEFFECT_SEPIA:
            SIV122DUWriteCmosSensor(0xB6, 0x04);//80); 
           // VWriteCmosSensor(0xB7, 0x60);
            //VWriteCmosSensor(0xB8, 0xA0);
            break;

        case MEFFECT_NEGATIVE:
            SIV122DUWriteCmosSensor(0xB6, 0x20);
            break;

        case MEFFECT_SEPIAGREEN:
            SIV122DUWriteCmosSensor(0xB6, 0x01);//80); 
            //SIV122DUWriteCmosSensor(0xB7, 0x50);
           // SIV122DUWriteCmosSensor(0xB8, 0x50);
            break;

        case MEFFECT_SEPIABLUE:
            SIV122DUWriteCmosSensor(0xB6, 0x02);//80); 
           // SIV122DUWriteCmosSensor(0xB7, 0xC0);
            //SIV122DUWriteCmosSensor(0xB8, 0x60);
            break;
			
		case MEFFECT_MONO: //B&W
            SIV122DUWriteCmosSensor(0xB6, 0x80);
			break;
        default:
            return FALSE;
    }
    return TRUE;

} /* SIV122DUSetParamEffect */

BOOL SIV122DUSetParamBanding(UINT16 para)
{

    SENSORDB("Banding %d\n",para);
	if(SIV122DUCurrentStatus.iBanding== para)
		return TRUE;

	SIV122DUCurrentStatus.iBanding = para;
    SIV122DUSetShutterStep();
    SIV122DUSetFrameCount(); 
    return TRUE;
} /* SIV122DUSetParamBanding */

BOOL SIV122DUSetParamEV(UINT16 para)
{
    SENSORDB("Exporsure %d\n",para);
	if(SIV122DUCurrentStatus.iEV== para)
		return ;

	SIV122DUCurrentStatus.iEV = para;
	
    SIV122DUSetPage(4);
    switch (para)
    {
        case AE_EV_COMP_n13:
      SIV122DUWriteCmosSensor(0xAB,0xC0);
            break;

        case AE_EV_COMP_n10:
      SIV122DUWriteCmosSensor(0xAB,0xB0);
            break;

        case AE_EV_COMP_n07:
      SIV122DUWriteCmosSensor(0xAB,0xA0);
            break;

        case AE_EV_COMP_n03:
      SIV122DUWriteCmosSensor(0xAB,0x90);
            break;

        case AE_EV_COMP_00:
			SIV122DUWriteCmosSensor(0xAB,0x00);
            break;

        case AE_EV_COMP_03:
      SIV122DUWriteCmosSensor(0xAB,0x10);
            break;

        case AE_EV_COMP_07:
      SIV122DUWriteCmosSensor(0xAB,0x20);
            break;

        case AE_EV_COMP_10:
      SIV122DUWriteCmosSensor(0xAB,0x30);
            break;

        case AE_EV_COMP_13:
      SIV122DUWriteCmosSensor(0xAB,0x40);
            break;

        default:
            return FALSE;
    }
    return TRUE;
} /* SIV122DUSetParamEV */



UINT32 SIV122DUYUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
	switch (iCmd) {
	case FID_SCENE_MODE:	    
	    if (iPara == SCENE_MODE_OFF)
	    {
	        SIV122DUNightMode(FALSE); 
	    }
	    else if (iPara == SCENE_MODE_NIGHTSCENE)
	    {
           SIV122DUNightMode(TRUE); 
	    }	    
	    break; 	
		
	case FID_AWB_MODE:
		SIV122DUSetParamWB(iPara);
		break;
		
	case FID_COLOR_EFFECT:
		SIV122DUSetParamEffect(iPara);
		break;
		
	case FID_AE_EV:
		SIV122DUSetParamEV(iPara);
		break;
		
	case FID_AE_FLICKER:
		SIV122DUSetParamBanding(iPara);
		break;
		
    case FID_AE_SCENE_MODE: 
		SIV122DUSetParamAE(iPara);
		 break;
		 
	default:
		break;
	}
	return TRUE;
}   /* SIV122DUYUVSensorSetting */





UINT32 SIV122DUFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 siv122duSensorRegNumber;
    UINT32 i;
	SENSORDB("Ronlus siv122duFeatureControl.---FeatureId = %d\r\n",FeatureId);
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

    RETAILMSG(1, (_T("gaiyang siv122duFeatureControl FeatureId=%d\r\n"), FeatureId));

    switch (FeatureId)
    {
    case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=VGA_PERIOD_PIXEL_NUMS;
        *pFeatureReturnPara16=VGA_PERIOD_LINE_NUMS;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=(VGA_PERIOD_PIXEL_NUMS)+SIV122DUCurrentStatus.iHblank;
        *pFeatureReturnPara16=(VGA_PERIOD_LINE_NUMS)+SIV122DUCurrentStatus.iVblank;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        *pFeatureReturnPara32 =SIV122DUCurrentStatus.iPclk;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
      SIV122DUNightMode((BOOL) *pFeatureData16);
        break;
    case SENSOR_FEATURE_SET_GAIN:
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        siv122du_isp_master_clock=*pFeatureData32;
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        SIV122DUWriteCmosSensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = SIV122DUReadCmosSensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &siv122duSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
        *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
        break;
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_GET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_GROUP_COUNT:
    case SENSOR_FEATURE_GET_GROUP_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
        break;
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
        SIV122DUGetSensorID(pFeatureReturnPara32); 
        break; 
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
        // if EEPROM does not exist in camera module.
        *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_YUV_CMD:
        SIV122DUYUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
        break;
    default:
        break;
	}
	return ERROR_NONE;
}	/* SIV122DUFeatureControl() */

SENSOR_FUNCTION_STRUCT	SensorFuncSIV122DU=
{
	SIV122DUOpen,
	SIV122DUGetInfo,
	SIV122DUGetResolution,
	SIV122DUFeatureControl,
	SIV122DUControl,
	SIV122DUClose
};


UINT32 SIV122DU_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	SENSORDB("Ronlus siv122du_YUV_SensorInit\r\n");
	if (pfFunc!=NULL)
	{
		 SENSORDB("Ronlussiv122du_YUV_SensorInit fun_config success\r\n");
		*pfFunc=&SensorFuncSIV122DU;
	}
	return ERROR_NONE;
}	/* SensorInit() */

