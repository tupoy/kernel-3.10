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
 *   sensor.c
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 * Author: 
 * -------
 *   Anyuan Huang (MTK70663)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
                  
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/io.h> 
//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "siv121duyuv_Sensor.h"
#include "siv121duyuv_Camera_Sensor_para.h"
#include "siv121duyuv_CameraCustomized.h"
#ifdef SIV121DU_LOAD_FROM_T_FLASH
//#include "med_utility.h"
//#include "fs_type.h"
#endif

    
   
#define Sleep(ms) mdelay(ms)	//wxu add

#define SIV121DUYUV_DEBUG

#define PK_DBG(a,...)

#ifdef SIV121DUYUV_DEBUG
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
} SIV121DUStatus;

#ifdef SIV121DU_LOAD_FROM_T_FLASH
	SIV121DU_initial_set_struct SIV121DU_Init_Reg[1000];
	WCHAR SIV121DU_set_file_name[256] = {0};
#endif


#define Sleep(ms) mdelay(ms)

/* Global Valuable */
SIV121DUStatus SIV121DUCurrentStatus;
MSDK_SENSOR_CONFIG_STRUCT SIV121DUSensorConfigData;

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 SIV121DUWriteCmosSensor(kal_uint32 addr, kal_uint32 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) ,(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 2,SIV121DU_WRITE_ID);
}

kal_uint16 SIV121DUReadCmosSensor(kal_uint32 addr)
{
	char puGetByte=0;
    char puSendCmd = (char)(addr & 0xFF);
	iReadRegI2C(&puSendCmd , 1, &puGetByte,1,SIV121DU_WRITE_ID);
    return puGetByte;
}

static void SIV121DUSetPage(kal_uint8 iPage)
{   
	
	if(SIV121DUCurrentStatus.iCurrentPage == iPage)
		return ;

	SIV121DUCurrentStatus.iCurrentPage = iPage;
    SIV121DUWriteCmosSensor(0x00,iPage);
}

static void SIV121DUInitialPara(void)
{
	SIV121DUCurrentStatus.bNightMode = KAL_FALSE;
	SIV121DUCurrentStatus.iWB = AWB_MODE_AUTO;
	SIV121DUCurrentStatus.iAE = AE_MODE_AUTO;
	SIV121DUCurrentStatus.iEffect = MEFFECT_OFF;
	SIV121DUCurrentStatus.iBanding = SIV121DU_NUM_50HZ;
	SIV121DUCurrentStatus.iEV = AE_EV_COMP_00;
	SIV121DUCurrentStatus.bFixFrameRate = KAL_FALSE;
	SIV121DUCurrentStatus.iMirror = IMAGE_NORMAL;
	SIV121DUCurrentStatus.iDummyPixel = 0xcd;//0x05; 	  
	SIV121DUCurrentStatus.bVideoMode = KAL_FALSE;
	SIV121DUCurrentStatus.iPclk =26;//24;
	
	SIV121DUCurrentStatus.iCurrentPage = 0;
	SIV121DUCurrentStatus.iControl = 0x00;
	SIV121DUCurrentStatus.iHblank = 0xcd;//0x05;
	SIV121DUCurrentStatus.iVblank = 0x19;
	SIV121DUCurrentStatus.iShutterStep = 0x82;//0x96;
	SIV121DUCurrentStatus.iFrameCount = 0x0A;

	SIV121DUCurrentStatus.MinFpsNormal = SIV121DU_FPS(10);
	SIV121DUCurrentStatus.MinFpsNight =  SIV121DUCurrentStatus.MinFpsNormal >> 1;
}


       
static void SIV121DUInitialSetting(void)
{
    SIV121DUWriteCmosSensor(0x00, 0x00);
	SIV121DUWriteCmosSensor(0x00, 0x01);
	SIV121DUWriteCmosSensor(0x03, 0x08);
	Sleep(20);
	SIV121DUWriteCmosSensor(0x00, 0x01);
	SIV121DUWriteCmosSensor(0x03, 0x08);
	Sleep(20);
	
	SIV121DUWriteCmosSensor(0x00, 0x00);
	SIV121DUWriteCmosSensor(0x03, 0x04);
	SIV121DUWriteCmosSensor(0x10, 0x86);
	SIV121DUWriteCmosSensor(0x11, 0x74);	

	SIV121DUWriteCmosSensor(0x00, 0x01);
	SIV121DUWriteCmosSensor(0x04, 0x00); 
	SIV121DUWriteCmosSensor(0x06, 0x04); 

	SIV121DUWriteCmosSensor(0x10, 0x11); 
	SIV121DUWriteCmosSensor(0x11, 0x25); 
	SIV121DUWriteCmosSensor(0x12, 0x21);

	SIV121DUWriteCmosSensor(0x17, 0x94); //ABS 1.74V
	SIV121DUWriteCmosSensor(0x18, 0x00);

#if 0
	SIV121DUWriteCmosSensor(0x20, 0x00);
	SIV121DUWriteCmosSensor(0x21, 0x05);
	SIV121DUWriteCmosSensor(0x22, 0x01);
	SIV121DUWriteCmosSensor(0x23, 0x69);
	#else
	
	SIV121DUWriteCmosSensor(0x20, 0x00);
	SIV121DUWriteCmosSensor(0x21, 0xCD);
	SIV121DUWriteCmosSensor(0x22, 0x01);
	SIV121DUWriteCmosSensor(0x23, 0x19);

#endif
	SIV121DUWriteCmosSensor(0x40, 0x0F); 
	SIV121DUWriteCmosSensor(0x41, 0x90);//0x90
	SIV121DUWriteCmosSensor(0x42, 0xd2);//0xd2
	SIV121DUWriteCmosSensor(0x43, 0x00);

	// AE
	SIV121DUWriteCmosSensor(0x00, 0x02);
	SIV121DUWriteCmosSensor(0x10, 0x84);
	SIV121DUWriteCmosSensor(0x11, 0x10);		
	SIV121DUWriteCmosSensor(0x12, 0x78); // D65 target 
	SIV121DUWriteCmosSensor(0x14, 0x76); // A target
	SIV121DUWriteCmosSensor(0x34, 0x82);//96);
	SIV121DUWriteCmosSensor(0x40, 0x40); // Max x6
	SIV121DUWriteCmosSensor(0x44, 0x08); // Max x6
	/*
	SIV121DUWriteCmosSensor(0x41, 0x28);
	SIV121DUWriteCmosSensor(0x42, 0x28);
	SIV121DUWriteCmosSensor(0x44, 0x08);
	SIV121DUWriteCmosSensor(0x45, 0x08);
	SIV121DUWriteCmosSensor(0x46, 0x15);
	SIV121DUWriteCmosSensor(0x47, 0x1c);
	SIV121DUWriteCmosSensor(0x48, 0x20);
	SIV121DUWriteCmosSensor(0x49, 0x21);
	SIV121DUWriteCmosSensor(0x4a, 0x23);
	SIV121DUWriteCmosSensor(0x4b, 0x24);
	SIV121DUWriteCmosSensor(0x4c, 0x25);
	SIV121DUWriteCmosSensor(0x4d, 0x28);
	SIV121DUWriteCmosSensor(0x4e, 0x1c);
	SIV121DUWriteCmosSensor(0x4f, 0x15);
	SIV121DUWriteCmosSensor(0x50, 0x12);
	SIV121DUWriteCmosSensor(0x51, 0x10);
	SIV121DUWriteCmosSensor(0x52, 0x0e);
	SIV121DUWriteCmosSensor(0x53, 0x0c);
	SIV121DUWriteCmosSensor(0x54, 0x0b);
	SIV121DUWriteCmosSensor(0x55, 0x0a);
	SIV121DUWriteCmosSensor(0x44, 0x08);
	SIV121DUWriteCmosSensor(0x5f, 0x01);
	SIV121DUWriteCmosSensor(0x90, 0x80);
	SIV121DUWriteCmosSensor(0x91, 0x80);
	*/

	// AWB
	SIV121DUWriteCmosSensor(0x00, 0x03);
	SIV121DUWriteCmosSensor(0x10, 0xd0);
	SIV121DUWriteCmosSensor(0x11, 0xc1);
	SIV121DUWriteCmosSensor(0x13, 0x80); //Cr target
	SIV121DUWriteCmosSensor(0x14, 0x80); //Cb target
	SIV121DUWriteCmosSensor(0x15, 0xe0); // R gain Top
	SIV121DUWriteCmosSensor(0x16, 0x7C); // R gain bottom 
	SIV121DUWriteCmosSensor(0x17, 0xe0); // B gain Top
	SIV121DUWriteCmosSensor(0x18, 0x80); // B gain bottom 0x80
	SIV121DUWriteCmosSensor(0x19, 0x8C); // Cr top value 0x90
	SIV121DUWriteCmosSensor(0x1a, 0x64); // Cr bottom value 0x70
	SIV121DUWriteCmosSensor(0x1b, 0x94); // Cb top value 0x90
	SIV121DUWriteCmosSensor(0x1c, 0x6c); // Cb bottom value 0x70
	SIV121DUWriteCmosSensor(0x1d, 0x94); // 0xa0
	SIV121DUWriteCmosSensor(0x1e, 0x6c); // 0x60
	SIV121DUWriteCmosSensor(0x20, 0xe8); // AWB luminous top value
	SIV121DUWriteCmosSensor(0x21, 0x30); // AWB luminous bottom value 0x20
	SIV121DUWriteCmosSensor(0x22, 0xb8);
	SIV121DUWriteCmosSensor(0x23, 0x10);
	SIV121DUWriteCmosSensor(0x25, 0x08);
	SIV121DUWriteCmosSensor(0x26, 0x0f);
	SIV121DUWriteCmosSensor(0x27, 0x60); // BRTSRT 
	SIV121DUWriteCmosSensor(0x28, 0x70); // BRTEND
	SIV121DUWriteCmosSensor(0x29, 0xb7); // BRTRGNBOT 
	SIV121DUWriteCmosSensor(0x2a, 0xa3); // BRTBGNTOP

	SIV121DUWriteCmosSensor(0x40, 0x01);
	SIV121DUWriteCmosSensor(0x41, 0x03);
	SIV121DUWriteCmosSensor(0x42, 0x08);
	SIV121DUWriteCmosSensor(0x43, 0x10);
	SIV121DUWriteCmosSensor(0x44, 0x13);
	SIV121DUWriteCmosSensor(0x45, 0x6a);
	SIV121DUWriteCmosSensor(0x46, 0xca);

	SIV121DUWriteCmosSensor(0x62, 0x80);
	SIV121DUWriteCmosSensor(0x63, 0x90); // R D30 to D20
	SIV121DUWriteCmosSensor(0x64, 0xd0); // B D30 to D20
	SIV121DUWriteCmosSensor(0x65, 0x98); // R D20 to D30
	SIV121DUWriteCmosSensor(0x66, 0xd0); // B D20 to D30

	// IDP
	SIV121DUWriteCmosSensor(0x00, 0x04);
	SIV121DUWriteCmosSensor(0x10, 0x7f);
	SIV121DUWriteCmosSensor(0x11, 0x1d);
	SIV121DUWriteCmosSensor(0x12, 0x0d);//2d);//fd
	SIV121DUWriteCmosSensor(0x14, 0x01);//fd
	// DPCBNR
	SIV121DUWriteCmosSensor(0x18, 0xbF);	// DPCNRCTRL
	SIV121DUWriteCmosSensor(0x19, 0x00);	// DPCTHV
	SIV121DUWriteCmosSensor(0x1A, 0x00);	// DPCTHVSLP
	SIV121DUWriteCmosSensor(0x1B, 0x00);	// DPCTHVDIFSRT
	//SIV121DUWriteCmosSensor(0x1C, 0x0f);	// DPCTHVDIFSLP
	//SIV121DUWriteCmosSensor(0x1d, 0xFF);	// DPCTHVMAX

	SIV121DUWriteCmosSensor(0x1E, 0x04);	// BNRTHV  0c
	SIV121DUWriteCmosSensor(0x1F, 0x08);	// BNRTHVSLPN 10
	SIV121DUWriteCmosSensor(0x20, 0x20);	// BNRTHVSLPD
	SIV121DUWriteCmosSensor(0x21, 0x00);	// BNRNEICNT
	SIV121DUWriteCmosSensor(0x22, 0x08);	// STRTNOR
	SIV121DUWriteCmosSensor(0x23, 0x38);	// STRTDRK
    SIV121DUWriteCmosSensor(0x24, 0x00);
	// Gamma
#if 1
	SIV121DUWriteCmosSensor(0x31, 0x03); //0x08
	SIV121DUWriteCmosSensor(0x32, 0x0B); //0x10
	SIV121DUWriteCmosSensor(0x33, 0x1E); //0x1B
	SIV121DUWriteCmosSensor(0x34, 0x46); //0x37
	SIV121DUWriteCmosSensor(0x35, 0x62); //0x4D
	SIV121DUWriteCmosSensor(0x36, 0x78); //0x60
	SIV121DUWriteCmosSensor(0x37, 0x8b); //0x72
	SIV121DUWriteCmosSensor(0x38, 0x9B); //0x82
	SIV121DUWriteCmosSensor(0x39, 0xA8); //0x91
	SIV121DUWriteCmosSensor(0x3a, 0xb6); //0xA0
	SIV121DUWriteCmosSensor(0x3b, 0xcc); //0xBA
	SIV121DUWriteCmosSensor(0x3c, 0xDF); //0xD3
	SIV121DUWriteCmosSensor(0x3d, 0xf0); //0xEA
#else
	SIV121DUWriteCmosSensor(0x31, 0x02); //0x08
	SIV121DUWriteCmosSensor(0x32, 0x09); //0x10
	SIV121DUWriteCmosSensor(0x33, 0x20); //0x1B
	SIV121DWriteCmosSensor(0x34, 0x44); //0x37
	SIV121DWriteCmosSensor(0x35, 0x62); //0x4D
	SIV121DWriteCmosSensor(0x36, 0x7c); //0x60
	SIV121DWriteCmosSensor(0x37, 0x8f); //0x72
	SIV121DWriteCmosSensor(0x38, 0x9f); //0x82
	SIV121DWriteCmosSensor(0x39, 0xae); //0x91
	SIV121DWriteCmosSensor(0x3a, 0xba); //0xA0
	SIV121DWriteCmosSensor(0x3b, 0xcf); //0xBA
	SIV121DWriteCmosSensor(0x3c, 0xe1); //0xD3
	SIV121DWriteCmosSensor(0x3d, 0xf0); //0xEA
#endif

	// Shading Register Setting 				 
	SIV121DUWriteCmosSensor(0x40, 0x06);						   
	SIV121DUWriteCmosSensor(0x41, 0x44);						   
	SIV121DUWriteCmosSensor(0x42, 0x43);						   
	SIV121DUWriteCmosSensor(0x43, 0x20);						   
	SIV121DUWriteCmosSensor(0x44, 0x11); // left R gain[7:4], right R gain[3:0] 						
	SIV121DUWriteCmosSensor(0x45, 0x11); // top R gain[7:4], bottom R gain[3:0] 						 
	SIV121DUWriteCmosSensor(0x46, 0x00); // left G gain[7:4], right G gain[3:0] 	  
	SIV121DUWriteCmosSensor(0x47, 0x00); // top G gain[7:4], bottom G gain[3:0] 	  
	SIV121DUWriteCmosSensor(0x48, 0x00); // left B gain[7:4], right B gain[3:0] 	  
	SIV121DUWriteCmosSensor(0x49, 0x00); // top B gain[7:4], bottom B gain[3:0] 	 
	SIV121DUWriteCmosSensor(0x4a, 0x04); // X-axis center high[3:2], Y-axis center high[1:0]	 
	SIV121DUWriteCmosSensor(0x4b, 0x48); // X-axis center low[7:0]						
	SIV121DUWriteCmosSensor(0x4c, 0xe8); // Y-axis center low[7:0]					   
	SIV121DUWriteCmosSensor(0x4d, 0x80); // Shading Center Gain


	// Interpolation
	SIV121DUWriteCmosSensor(0x60,  0x7F);
	SIV121DUWriteCmosSensor(0x61,  0x08);	// INTCTRL outdoor
#if 0
	// Color matrix (D65) - Daylight
	SIV121DUWriteCmosSensor(0x71, 0x3B);
	SIV121DUWriteCmosSensor(0x72, 0xC7);
	SIV121DUWriteCmosSensor(0x73, 0xFe);
	SIV121DUWriteCmosSensor(0x74, 0x10);
	SIV121DUWriteCmosSensor(0x75, 0x28);
	SIV121DUWriteCmosSensor(0x76, 0x08);
	SIV121DUWriteCmosSensor(0x77, 0xec);
	SIV121DUWriteCmosSensor(0x78, 0xCd);
	SIV121DUWriteCmosSensor(0x79, 0x47);

	// Color matrix (D20) - A
	SIV121DUWriteCmosSensor(0x83, 0x38); //0x3c 	
	SIV121DUWriteCmosSensor(0x84, 0xd1); //0xc6 	
	SIV121DUWriteCmosSensor(0x85, 0xf7); //0xff   
	SIV121DUWriteCmosSensor(0x86, 0x12); //0x12    
	SIV121DUWriteCmosSensor(0x87, 0x25); //0x24 	
	SIV121DUWriteCmosSensor(0x88, 0x09); //0x0a 	
	SIV121DUWriteCmosSensor(0x89, 0xed); //0xed   
	SIV121DUWriteCmosSensor(0x8a, 0xbb); //0xc2 	
	SIV121DUWriteCmosSensor(0x8b, 0x58); //0x51
#else
	// Color matrix (D65) - Daylight
	SIV121DUWriteCmosSensor(0x71, 0x39);
	SIV121DUWriteCmosSensor(0x72, 0xC8);
	SIV121DUWriteCmosSensor(0x73, 0xFf);
	SIV121DUWriteCmosSensor(0x74, 0x13);
	SIV121DUWriteCmosSensor(0x75, 0x25);
	SIV121DUWriteCmosSensor(0x76, 0x08);
	SIV121DUWriteCmosSensor(0x77, 0xf8);
	SIV121DUWriteCmosSensor(0x78, 0xC0);
	SIV121DUWriteCmosSensor(0x79, 0x48);

	// Color matrix (D20) - A
	SIV121DUWriteCmosSensor(0x83, 0x34); //0x3c 	
	SIV121DUWriteCmosSensor(0x84, 0xd4); //0xc6 	
	SIV121DUWriteCmosSensor(0x85, 0xf8); //0xff   
	SIV121DUWriteCmosSensor(0x86, 0x12); //0x12    
	SIV121DUWriteCmosSensor(0x87, 0x25); //0x24 	
	SIV121DUWriteCmosSensor(0x88, 0x09); //0x0a 	
	SIV121DUWriteCmosSensor(0x89, 0xf0); //0xed   
	SIV121DUWriteCmosSensor(0x8a, 0xbe); //0xc2 	
	SIV121DUWriteCmosSensor(0x8b, 0x54); //0x51
#endif
	SIV121DUWriteCmosSensor(0x8c, 0x10); //CMA select	  

	//G Edge
	SIV121DUWriteCmosSensor(0x90, 0x35); //Upper gain
	SIV121DUWriteCmosSensor(0x91, 0x48); //down gain
	SIV121DUWriteCmosSensor(0x92, 0x33); //[7:4] upper coring [3:0] down coring
	SIV121DUWriteCmosSensor(0x9a, 0x40);
	SIV121DUWriteCmosSensor(0x9b, 0x40);
	SIV121DUWriteCmosSensor(0x9c, 0x38); //edge suppress start
	SIV121DUWriteCmosSensor(0x9d, 0x30); //edge suppress slope

	SIV121DUWriteCmosSensor(0x9f, 0x26);
	SIV121DUWriteCmosSensor(0xa0, 0x11);

	SIV121DUWriteCmosSensor(0xa8, 0x11);
	SIV121DUWriteCmosSensor(0xa9, 0x12);
	SIV121DUWriteCmosSensor(0xaa, 0x12);

	SIV121DUWriteCmosSensor(0xb9, 0x28); // nightmode 38 at gain 0x48 5fps
	SIV121DUWriteCmosSensor(0xba, 0x41); // nightmode 80 at gain 0x48 5fps


	SIV121DUWriteCmosSensor(0xbf, 0x20);
	SIV121DUWriteCmosSensor(0xde, 0x80);
	/*
	SIV121DUWriteCmosSensor(0xc0, 0x24);	 
	SIV121DUWriteCmosSensor(0xc1, 0x00);	 
	SIV121DUWriteCmosSensor(0xc2, 0x80);	
	SIV121DUWriteCmosSensor(0xc3, 0x00);	 
	SIV121DUWriteCmosSensor(0xc4, 0xe0);	 
	SIV121DUWriteCmosSensor(0xcb,0x00);
	SIV121DUWriteCmosSensor(0xde, 0x80);
	*/

	SIV121DUWriteCmosSensor(0xe5, 0x15); //MEMSPDA
	SIV121DUWriteCmosSensor(0xe6, 0x02); //MEMSPDB
	SIV121DUWriteCmosSensor(0xe7, 0x04); //MEMSPDC
 

	//Sensor On  
	SIV121DUWriteCmosSensor(0x00, 0x01);
	SIV121DUWriteCmosSensor(0x03, 0x01); // SNR Enable
}   /* SIV121DUInitialSetting */   /* SIV121DUInitialSetting */


/*************************************************************************
* FUNCTION
*    SIV121DUHalfAdjust
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
__inline static kal_uint32 SIV121DUHalfAdjust(kal_uint32 dividend, kal_uint32 divisor)
{
  return (dividend * 2 + divisor) / (divisor * 2); /* that is [dividend / divisor + 0.5]*/
}

/*************************************************************************
* FUNCTION
*   SIV121DUSetShutterStep
*
* DESCRIPTION
*   This function is to calculate & set shutter step register .
*
*************************************************************************/
static void SIV121DUSetShutterStep(void)
{       
    const kal_uint8 banding = SIV121DUCurrentStatus.iBanding == AE_FLICKER_MODE_50HZ ? SIV121DU_NUM_50HZ : SIV121DU_NUM_60HZ;
    const kal_uint16 shutter_step = SIV121DUHalfAdjust(SIV121DUCurrentStatus.iPclk * SIV121DU_CLK_1MHZ / 2, (SIV121DUCurrentStatus.iHblank + SIV121DU_PERIOD_PIXEL_NUMS) * banding);

	if(SIV121DUCurrentStatus.iShutterStep == shutter_step)
		return ;

	SIV121DUCurrentStatus.iShutterStep = shutter_step;
    ASSERT(shutter_step <= 0xFF);    
    /* Block 1:0x34  shutter step*/
    SIV121DUSetPage(2);
    SIV121DUWriteCmosSensor(0x34,0x82);//shutter_step);
	
    SENSORDB("Set Shutter Step:%x\n",shutter_step);
}/* SIV121DUSetShutterStep */


            
/*************************************************************************
* FUNCTION
*   SIV121DUSetFrameCount
*
* DESCRIPTION
*   This function is to set frame count register .
*
*************************************************************************/
static void SIV121DUSetFrameCount(void)
{    
    kal_uint16 Frame_Count,min_fps = 60;
    kal_uint8 banding = SIV121DUCurrentStatus.iBanding == AE_FLICKER_MODE_50HZ ? SIV121DU_NUM_50HZ : SIV121DU_NUM_60HZ;

	min_fps = SIV121DUCurrentStatus.bNightMode ? SIV121DUCurrentStatus.MinFpsNight : SIV121DUCurrentStatus.MinFpsNormal;
    Frame_Count = banding * SIV121DU_FRAME_RATE_UNIT / min_fps;

	if(SIV121DUCurrentStatus.iFrameCount == Frame_Count)
		return ;

	SIV121DUCurrentStatus.iFrameCount = Frame_Count;
	
    SENSORDB("min_fps:%d,Frame_Count:%x\n",min_fps/SIV121DU_FRAME_RATE_UNIT,Frame_Count);
    /*Block 01: 0x11  Max shutter step,for Min frame rate */
    SIV121DUSetPage(2);
    SIV121DUWriteCmosSensor(0x11,Frame_Count&0xFF);    
}/* SIV121DUSetFrameCount */

/*************************************************************************
* FUNCTION
*   SIV121DUConfigBlank
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
static void SIV121DUConfigBlank(kal_uint16 hblank,kal_uint16 vblank)
{    
    SENSORDB("hblank:%x,vblank:%x\n",hblank,vblank);
   /********************************************    
    *   Register :0x20 - 0x22
    *  Block 00
    *  0x20  [7:4]:HBANK[9:8]; 0x20  [3:0]:VBANK[9:8]
    *  0x21 HBANK[7:0]
    *  0x23 VBANK[7:0]  
    ********************************************/
	if((SIV121DUCurrentStatus.iHblank == hblank) && (SIV121DUCurrentStatus.iVblank == vblank) )
	   return ;

	SIV121DUCurrentStatus.iHblank = hblank;
	SIV121DUCurrentStatus.iVblank = vblank;
	ASSERT((hblank <= SIV121DU_BLANK_REGISTER_LIMITATION) && (vblank <= SIV121DU_BLANK_REGISTER_LIMITATION)); //??????????? ,?????,???????
	SIV121DUSetPage(1);
	#if 0
	SIV121DUWriteCmosSensor(0x20,((hblank>>4)&0xF0)|((vblank>>8)&0x0F));
	SIV121DUWriteCmosSensor(0x21,hblank & 0xFF);
	SIV121DUWriteCmosSensor(0x23,vblank & 0xFF);
	#endif
	SIV121DUWriteCmosSensor(0x20, 0x00);
    SIV121DUWriteCmosSensor(0x21, 0xCD);
	//SIV121DUWriteCmosSensor(0x22, 0x01);
	SIV121DUWriteCmosSensor(0x23, 0x19);
	SIV121DUSetShutterStep();
}   /* SIV121DUConfigBlank */

/*************************************************************************
* FUNCTION
*    SIV121DUCalFps
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
static void SIV121DUCalFps(void)
{
    kal_uint16 Line_length,Dummy_Line,Dummy_Pixel;
  kal_uint16 max_fps = 250;

    Line_length = SIV121DUCurrentStatus.iDummyPixel + SIV121DU_PERIOD_PIXEL_NUMS; 
	
	if (SIV121DUCurrentStatus.bVideoMode == KAL_TRUE)
	{		
		max_fps = SIV121DUCurrentStatus.bNightMode ? SIV121DUCurrentStatus.MinFpsNight: SIV121DUCurrentStatus.MinFpsNormal;
	}
	else
	{		
		max_fps = SIV121DU_FPS(25);
	}
	
    Dummy_Line = SIV121DUCurrentStatus.iPclk * SIV121DU_CLK_1MHZ * SIV121DU_FRAME_RATE_UNIT / (2 * Line_length * max_fps) - SIV121DU_PERIOD_LINE_NUMS; 
    if(Dummy_Line > SIV121DU_BLANK_REGISTER_LIMITATION)
    {
        Dummy_Line = SIV121DU_BLANK_REGISTER_LIMITATION;
        Line_length = SIV121DUCurrentStatus.iPclk * SIV121DU_CLK_1MHZ * SIV121DU_FRAME_RATE_UNIT / (2 * (Dummy_Line + SIV121DU_PERIOD_LINE_NUMS) * max_fps);
    }
    Dummy_Pixel = Line_length -  SIV121DU_PERIOD_PIXEL_NUMS;
	
    SENSORDB("max_fps:%d\n",max_fps/SIV121DU_FRAME_RATE_UNIT);
    SENSORDB("Dummy Pixel:%x,Hblank:%x,Vblank:%x\n",SIV121DUCurrentStatus.iDummyPixel,Dummy_Pixel,Dummy_Line);
    SIV121DUConfigBlank((Dummy_Pixel > 0) ? Dummy_Pixel : 0, (Dummy_Line > 0) ? Dummy_Line : 0);
    SIV121DUSetShutterStep();
}


/*************************************************************************
* FUNCTION
*   SIV121DUFixFrameRate
*
* DESCRIPTION
*   This function fix the frame rate of image sensor.
*
*************************************************************************/
static void SIV121DUFixFrameRate(kal_bool bEnable)
{
	if(SIV121DUCurrentStatus.bFixFrameRate == bEnable)
		return ;

	SIV121DUCurrentStatus.bFixFrameRate = bEnable;
    if(bEnable == KAL_TRUE)
    {   //fix frame rate
        SIV121DUCurrentStatus.iControl |= 0xC0;
    }
    else
    {        
        SIV121DUCurrentStatus.iControl &= 0x3F;
    }
	SIV121DUSetPage(1);
	SIV121DUWriteCmosSensor(0x04,SIV121DUCurrentStatus.iControl);
}   /* SIV121DUFixFrameRate */

/*************************************************************************
* FUNCTION
*   SIV121DUHVmirror
*
* DESCRIPTION
*   This function config the HVmirror of image sensor.
*
*************************************************************************/
static void SIV121DUHVmirror(kal_uint8 HVmirrorType)
{   
	if(SIV121DUCurrentStatus.iMirror == HVmirrorType)
		return ;

	SIV121DUCurrentStatus.iMirror = HVmirrorType;
    SIV121DUCurrentStatus.iControl = SIV121DUCurrentStatus.iControl & 0xFC;  
    switch (HVmirrorType) 
    {
        case IMAGE_H_MIRROR:
            SIV121DUCurrentStatus.iControl |= 0x01;
            break;
            
        case IMAGE_V_MIRROR:
            SIV121DUCurrentStatus.iControl |= 0x02;
            break;
                
        case IMAGE_HV_MIRROR:
            SIV121DUCurrentStatus.iControl |= 0x03;
            break;
		case IMAGE_NORMAL:
        default:
            SIV121DUCurrentStatus.iControl |= 0x00;
    }    
    SIV121DUSetPage(1);
    SIV121DUWriteCmosSensor(0x04,SIV121DUCurrentStatus.iControl);
}   /* SIV121HVmirror */

/*************************************************************************
* FUNCTION
*	SIV121DUNightMode
*
* DESCRIPTION
*	This function night mode of SIV121DU.
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
void SIV121DUNightMode(kal_bool enable)
{
    SENSORDB("NightMode %d",enable);

    SIV121DUCurrentStatus.bNightMode = enable;

  if ( SIV121DUCurrentStatus.bVideoMode == KAL_TRUE)// camera mode
    return ;
        
		SIV121DUSetPage(2);
		//SIV121DUWriteCmosSensor(0x40,0x64);//Max Analog Gain Value @ Shutter step = Max Shutter step  0x7D
        if (SIV121DUCurrentStatus.bNightMode == KAL_TRUE)
        {   /* camera night mode */
          //  PK_DBG("camera night mode\n");
           // SIV121DUSetPage(1);
          
		   SIV121DUWriteCmosSensor(0x11,0x14);
           SIV121DUWriteCmosSensor(0x40,0x50); //Max Analog Gain Value @ Shutter step = Max Shutter step  0x7D
            //SIV121DUSetPage(3);
            //SIV121DUWriteCmosSensor(0xAB,0x98); //Brightness Control 0x11
            //SIV121DUWriteCmosSensor(0xB9,0x18); //Color Suppression Change Start State  0x18           
            //SIV121DUWriteCmosSensor(0xBA,0x20); //Slope
        }
        else
        {   /* camera normal mode */
          //  PK_DBG("camera normal mode\n");
            
		    SIV121DUWriteCmosSensor(0x11,0x10);
            SIV121DUWriteCmosSensor(0x40,0x40);// 0x7F
            //SIV121DUSetPage(3);
            //SIV121DUWriteCmosSensor(0xAB,0x98); //0x04 
            //SIV121DUWriteCmosSensor(0xB9,0x18);            
            //SIV121DUWriteCmosSensor(0xBA,0x20); //Slope
        }
		SIV121DUSetFrameCount();    
}  /* SIV121DUNightMode */

#ifdef SIV121DU_LOAD_FROM_T_FLASH
/*************************************************************************
* FUNCTION
*	SIV121DU_Initialize_from_T_Flash
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
static kal_uint8 SIV121DU_Initialize_from_T_Flash()
{
	FS_HANDLE fp = -1;				/* Default, no file opened. */
	kal_uint8 *data_buff = NULL;
	kal_uint8 *curr_ptr = NULL;
	kal_uint32 file_size = 0;
	kal_uint32 bytes_read = 0;
	kal_uint32 i = 0, j = 0;
	kal_uint8 func_ind[3] = {0};	/* REG or DLY */

	kal_mem_cpy(SIV121DU_set_file_name, L"C:\\SIV121DU_Initialize_Setting.Bin", sizeof(L"C:\\SIV121DU_Initialize_Setting.Bin"));

	/* Search the setting file in all of the user disk. */
	curr_ptr = (kal_uint8 *)SIV121DU_set_file_name;
	
	while (fp < 0)
	{
		if ((*curr_ptr >= 'c' && *curr_ptr <= 'z') || (*curr_ptr >= 'C' && *curr_ptr <= 'Z'))
		{
			fp = FS_Open(SIV121DU_set_file_name, FS_READ_ONLY);
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
	#ifdef __SIV121DU_DEBUG_TRACE__
		kal_wap_trace(MOD_ENG, TRACE_INFO, "!!! Warning, Can't find the initial setting file!!!");
	#endif
	
		return 0;
	}

	FS_GetFileSize(fp, &file_size);
	if (file_size < 20)
	{
	#ifdef __SIV121DU_DEBUG_TRACE__
		kal_wap_trace(MOD_ENG, TRACE_INFO, "!!! Warning, Invalid setting file!!!");
	#endif
	
		return 0;			/* Invalid setting file. */
	}

	data_buff = med_alloc_ext_mem(file_size);
	if (data_buff == NULL)
	{
	#ifdef __SIV121DU_DEBUG_TRACE__
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
			SIV121DU_Init_Reg[i].op_code = SIV121DU_OP_CODE_REG;
			
			SIV121DU_Init_Reg[i].init_reg = strtol((const char *)curr_ptr, NULL, 16);
			curr_ptr += SIV121DU_REG_SKIP;	/* Skip "0x0000, " */
			
			SIV121DU_Init_Reg[i].init_val = strtol((const char *)curr_ptr, NULL, 16);
			curr_ptr += SIV121DU_VAL_SKIP;	/* Skip "0x0000);" */
		}
		else									/* DLY */
		{
			/* Need add delay for this setting. */
			SIV121DU_Init_Reg[i].op_code = SIV121DU_OP_CODE_DLY;
			
			SIV121DU_Init_Reg[i].init_reg = 0xFF;
			SIV121DU_Init_Reg[i].init_val = strtol((const char *)curr_ptr, NULL, 10);	/* Get the delay ticks, the delay should less then 50 */
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
	SIV121DU_Init_Reg[i].op_code = SIV121DU_OP_CODE_END;
	SIV121DU_Init_Reg[i].init_reg = 0xFF;
	SIV121DU_Init_Reg[i].init_val = 0xFF;
	i++;
	
#ifdef __SIV121DU_DEBUG_TRACE__
	kal_wap_trace(MOD_ENG, TRACE_INFO, "%d register read...", i-1);
#endif

	med_free_ext_mem((void **)&data_buff);
	FS_Close(fp);

#ifdef __SIV121DU_DEBUG_TRACE__
	kal_wap_trace(MOD_ENG, TRACE_INFO, "Start apply initialize setting.");
#endif
	/* Start apply the initial setting to sensor. */
	for (j=0; j<i; j++)
	{
		if (SIV121DU_Init_Reg[j].op_code == SIV121DU_OP_CODE_END)	/* End of the setting. */
		{
			break ;
		}
		else if (SIV121DU_Init_Reg[j].op_code == SIV121DU_OP_CODE_DLY)
		{
			kal_sleep_task(SIV121DU_Init_Reg[j].init_val);		/* Delay */
		}
		else if (SIV121DU_Init_Reg[j].op_code == SIV121DU_OP_CODE_REG)
		{
			SIV121DUWriteCmosSensor(SIV121DU_Init_Reg[j].init_reg, SIV121DU_Init_Reg[j].init_val);
		}
		else
		{
			ASSERT(0);
		}
	}

#ifdef __SIV121DU_DEBUG_TRACE__
	kal_wap_trace(MOD_ENG, TRACE_INFO, "%d register applied...", j);
#endif

	return 1;	

}
#endif


/*************************************************************************
* FUNCTION
*	SIV121DUOpen
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
UINT32 SIV121DUOpen(void)
{
	volatile signed char i;
	kal_uint16 sensor_id=0;


	//  Read sensor ID to adjust I2C is OK?
    SIV121DUWriteCmosSensor(0x00,0x00);
	sensor_id = SIV121DUReadCmosSensor(0x01); //?ID
	PK_DBG("SIV121DU Sensor Read ID %x\n",sensor_id);
	
	if (sensor_id != SIV121DU_SENSOR_ID) 
	{
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	SIV121DUCurrentStatus.iSensorVersion  = SIV121DUReadCmosSensor(0x02);	
	PK_DBG("IV120D Sensor Version %x\n",SIV121DUCurrentStatus.iSensorVersion);
	//PK_DBG("IV120D Sensor Version %x\n",SIV121DUCurrentStatus.iSensorVersion);
	//SIV121DU_set_isp_driving_current(3);
	SIV121DUInitialPara();

    
#ifdef SIV121DU_LOAD_FROM_T_FLASH
		if (SIV121DU_Initialize_from_T_Flash())		/* For debug use. */
		{
			/* If load from t-flash success, then do nothing. */
		}
		else	/* Failed, load from the image load. */
#endif
	SIV121DUInitialSetting();
	mdelay(10);
    SIV121DUCalFps();    

	return ERROR_NONE;
}	/* SIV121DUOpen() */
/*************************************************************************
* FUNCTION
*   SIV121DUGetSensorID
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
UINT32 SIV121DUGetSensorID(UINT32 *sensorID) 
{
	
	SENSORDB("SIV121DUGetSensorID\n");	
	//	Read sensor ID to adjust I2C is OK?
    SIV121DUWriteCmosSensor(0x00,0x00);
	*sensorID = SIV121DUReadCmosSensor(0x01);
	SENSORDB("SIV121DU Sensor Read ID %x\n",*sensorID);
	if (*sensorID != SIV121DU_SENSOR_ID) 
	{
	  return ERROR_SENSOR_CONNECT_FAIL;
	}
	
	return ERROR_NONE;
}



/*************************************************************************
* FUNCTION
*	SIV121DUClose
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
UINT32 SIV121DUClose(void)
{
//	CISModulePowerOn(FALSE);
	SIV121DUWriteCmosSensor(0x00, 0x01);
	SIV121DUWriteCmosSensor(0x03, 0x08);

	return ERROR_NONE;
}	/* SIV121DUClose() */

/*************************************************************************
* FUNCTION
*	SIV121DUPreview
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
UINT32 SIV121DUPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	SENSORDB("SIV121DUPreview\r\n");

	SENSORDB("SensorOperationMode =%d\r\n",sensor_config_data->SensorOperationMode);
    /* ==Camera Preview, MT6516 use 26MHz PCLK, 30fps == */
	SENSORDB("Camera preview\r\n");
    //4  <1> preview of capture PICTURE
	//SIV121DUFixFrameRate(KAL_FALSE);
	SIV121DUCurrentStatus.MinFpsNormal = SIV121DU_FPS(6);
	SIV121DUCurrentStatus.MinFpsNight =  SIV121DUCurrentStatus.MinFpsNormal >> 1;	
	SIV121DUCurrentStatus.bVideoMode =  KAL_FALSE;
	 
    //4 <2> set mirror and flip
    SIV121DUHVmirror(sensor_config_data->SensorImageMirror);

    //4 <3> set dummy pixel, dummy line will calculate from frame rate
	/* SIV121DUCurrentStatus.iDummyPixel = 0x1d; */	  
	 SIV121DUCurrentStatus.iDummyPixel = 0xcd; 
	// copy sensor_config_data
	memcpy(&SIV121DUSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
  	return ERROR_NONE;
}	/* SIV121DUPreview() */

UINT32 SIV121DUGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	//pSensorResolution->SensorFullWidth = SIV121DU_IMAGE_SENSOR_PV_WIDTH+2; //add workaround for VGA sensor
	pSensorResolution->SensorFullWidth = SIV121DU_IMAGE_SENSOR_PV_WIDTH; //+2; //add workaround for VGA sensor
	pSensorResolution->SensorFullHeight = SIV121DU_IMAGE_SENSOR_PV_HEIGHT;
	pSensorResolution->SensorPreviewWidth = SIV121DU_IMAGE_SENSOR_PV_WIDTH;
	pSensorResolution->SensorPreviewHeight = SIV121DU_IMAGE_SENSOR_PV_HEIGHT;
	pSensorResolution->SensorVideoWidth = SIV121DU_IMAGE_SENSOR_PV_WIDTH;
	pSensorResolution->SensorVideoHeight = SIV121DU_IMAGE_SENSOR_PV_HEIGHT;
	return ERROR_NONE;
}	/* SIV121DUGetResolution() */

UINT32 SIV121DUGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX = SIV121DU_IMAGE_SENSOR_PV_WIDTH;
	pSensorInfo->SensorPreviewResolutionY = SIV121DU_IMAGE_SENSOR_PV_HEIGHT;
	pSensorInfo->SensorFullResolutionX = SIV121DU_IMAGE_SENSOR_PV_WIDTH;
	pSensorInfo->SensorFullResolutionY = SIV121DU_IMAGE_SENSOR_PV_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=25;
	pSensorInfo->SensorStillCaptureFrameRate=25;
	pSensorInfo->SensorWebCamCaptureFrameRate=25;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YVYU;
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
			pSensorInfo->SensorClockFreq=26; //??MCLK
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX = 1; 
			pSensorInfo->SensorGrabStartY = 1;		   
			break;
	}
	memcpy(pSensorConfigData, &SIV121DUSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* SIV121DUGetInfo() */

          
UINT32 SIV121DUControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	SENSORDB("SIV121DUControl()  ScenarioId =%d\r\n",ScenarioId);
// both preview and capture ScenarioId=3;
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			SIV121DUPreview(pImageWindow, pSensorConfigData);
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			SIV121DUPreview(pImageWindow, pSensorConfigData);
			break;
		//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		//case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		default:
			SIV121DUPreview(pImageWindow, pSensorConfigData);
			//PK_DBG("pImageWindow ,pSensorConfigData\r\n",pImageWindow,pSensorConfigData);
			//PK_DBG("SIV121DUControl\r\n" );
			break;
	}
	return TRUE;
}	/* SIV121DUControl() */

BOOL SIV121DUSetParamWB(UINT16 para)
{
    SENSORDB("WB %d\n",para);
	if(SIV121DUCurrentStatus.iWB== para)
		return ;

	SIV121DUCurrentStatus.iWB = para;
    SIV121DUSetPage(3);
    switch (para)
    {
        case AWB_MODE_OFF:
            SIV121DUWriteCmosSensor(0x10, 0x00);
            break;
			
        case AWB_MODE_AUTO:
            SIV121DUWriteCmosSensor(0x10, 0xD0);
            break;

        case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
	SIV121DUWriteCmosSensor(0x10, 0xD5); 
	SIV121DUWriteCmosSensor(0x72, 0xb4);
	SIV121DUWriteCmosSensor(0x73, 0x74);
            break;

        case AWB_MODE_DAYLIGHT: //sunny
	SIV121DUWriteCmosSensor(0x10, 0xD5); 
	SIV121DUWriteCmosSensor(0x72, 0xD8);
	SIV121DUWriteCmosSensor(0x73, 0x90);
            break;

        case AWB_MODE_INCANDESCENT: //office
	SIV121DUWriteCmosSensor(0x10, 0xD5); 
	SIV121DUWriteCmosSensor(0x72, 0x80);
	SIV121DUWriteCmosSensor(0x73, 0xE0);
            break;

    case AWB_MODE_TUNGSTEN: //home
	SIV121DUWriteCmosSensor(0x10, 0xD5); 
	SIV121DUWriteCmosSensor(0x72, 0xb8);
	SIV121DUWriteCmosSensor(0x73, 0xCC);
            break;
			
        case AWB_MODE_FLUORESCENT:
	SIV121DUWriteCmosSensor(0x10, 0xD5); 
	SIV121DUWriteCmosSensor(0x72, 0x78);
	SIV121DUWriteCmosSensor(0x73, 0xA0);
            break;

        default:
            return FALSE;
    }

    return TRUE;
} /* SIV121DUSetParamWB */

BOOL SIV121DUSetParamAE(UINT16 para)
{
    SENSORDB("AE %d\n",para);
	if(SIV121DUCurrentStatus.iAE== para)
		return ;

	SIV121DUCurrentStatus.iAE = para;
	SIV121DUSetPage(2);//??????AE,????????????AE??,????????????
	if(KAL_TRUE == para)
		SIV121DUWriteCmosSensor(0x10,0x80);
	else
		SIV121DUWriteCmosSensor(0x10,0x00);

    return TRUE;
} /* SIV121DUSetParamAE */

BOOL SIV121DUSetParamEffect(UINT16 para)
{
    SENSORDB("Effect %d\n",para);
	if(SIV121DUCurrentStatus.iEffect== para)
		return ;

	SIV121DUCurrentStatus.iEffect = para;
    SIV121DUSetPage(4);
    switch (para)
    {
        case MEFFECT_OFF:
            SIV121DUWriteCmosSensor(0xB6, 0x00);
            break;

        case MEFFECT_SEPIA:
            SIV121DUWriteCmosSensor(0xB6, 0x04);//80); 
           // VWriteCmosSensor(0xB7, 0x60);
            //VWriteCmosSensor(0xB8, 0xA0);
            break;

        case MEFFECT_NEGATIVE:
            SIV121DUWriteCmosSensor(0xB6, 0x20);
            break;

        case MEFFECT_SEPIAGREEN:
            SIV121DUWriteCmosSensor(0xB6, 0x01);//80); 
            //SIV121DUWriteCmosSensor(0xB7, 0x50);
           // SIV121DUWriteCmosSensor(0xB8, 0x50);
            break;

        case MEFFECT_SEPIABLUE:
            SIV121DUWriteCmosSensor(0xB6, 0x02);//80); 
           // SIV121DUWriteCmosSensor(0xB7, 0xC0);
            //SIV121DUWriteCmosSensor(0xB8, 0x60);
            break;
			
		case MEFFECT_MONO: //B&W
            SIV121DUWriteCmosSensor(0xB6, 0x80);
			break;
        default:
            return FALSE;
    }
    return TRUE;

} /* SIV121DUSetParamEffect */

BOOL SIV121DUSetParamBanding(UINT16 para)
{

    SENSORDB("Banding %d\n",para);
	if(SIV121DUCurrentStatus.iBanding== para)
		return TRUE;

	SIV121DUCurrentStatus.iBanding = para;
    SIV121DUSetShutterStep();
    SIV121DUSetFrameCount(); 
    return TRUE;
} /* SIV121DUSetParamBanding */

BOOL SIV121DUSetParamEV(UINT16 para)
{
    SENSORDB("Exporsure %d\n",para);
	if(SIV121DUCurrentStatus.iEV== para)
		return ;

	SIV121DUCurrentStatus.iEV = para;
	
    SIV121DUSetPage(4);
    switch (para)
    {
        case AE_EV_COMP_n13:
      SIV121DUWriteCmosSensor(0xAB,0xC0);
            break;

        case AE_EV_COMP_n10:
      SIV121DUWriteCmosSensor(0xAB,0xB0);
            break;

        case AE_EV_COMP_n07:
      SIV121DUWriteCmosSensor(0xAB,0xA0);
            break;

        case AE_EV_COMP_n03:
      SIV121DUWriteCmosSensor(0xAB,0x90);
            break;

        case AE_EV_COMP_00:
			SIV121DUWriteCmosSensor(0xAB,0x00);
            break;

        case AE_EV_COMP_03:
      SIV121DUWriteCmosSensor(0xAB,0x10);
            break;

        case AE_EV_COMP_07:
      SIV121DUWriteCmosSensor(0xAB,0x20);
            break;

        case AE_EV_COMP_10:
      SIV121DUWriteCmosSensor(0xAB,0x30);
            break;

        case AE_EV_COMP_13:
      SIV121DUWriteCmosSensor(0xAB,0x40);
            break;

        default:
            return FALSE;
    }
    return TRUE;
} /* SIV121DUSetParamEV */



UINT32 SIV121DUYUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
	switch (iCmd) {
	case FID_SCENE_MODE:	    
	    if (iPara == SCENE_MODE_OFF)
	    {
	        SIV121DUNightMode(FALSE); 
	    }
	    else if (iPara == SCENE_MODE_NIGHTSCENE)
	    {
           SIV121DUNightMode(TRUE); 
	    }	    
	    break; 	
		
	case FID_AWB_MODE:
		SIV121DUSetParamWB(iPara);
		break;
		
	case FID_COLOR_EFFECT:
		SIV121DUSetParamEffect(iPara);
		break;
		
	case FID_AE_EV:
		SIV121DUSetParamEV(iPara);
		break;
		
	case FID_AE_FLICKER:
		SIV121DUSetParamBanding(iPara);
		break;
		
    case FID_AE_SCENE_MODE: 
		SIV121DUSetParamAE(iPara);
		 break;
		 
	default:
		break;
	}
	return TRUE;
}   /* SIV121DUYUVSensorSetting */

UINT32 SIV121DUYUVSetVideoMode(UINT16 u2FrameRate)
{
    SENSORDB("SetVideoMode %d\n",u2FrameRate);

	//SIV121DUFixFrameRate(KAL_TRUE);  
	SIV121DUCurrentStatus.bVideoMode = KAL_TRUE;
	SIV121DUCurrentStatus.MinFpsNormal = SIV121DU_FPS(6);
	SIV121DUCurrentStatus.MinFpsNight =  SIV121DUCurrentStatus.MinFpsNormal >> 1; 
    if (u2FrameRate == 12)
    {
		SIV121DUCurrentStatus.bNightMode = KAL_FALSE;
    }
    else if (u2FrameRate == 6)       
    {
		SIV121DUCurrentStatus.bNightMode = KAL_TRUE;
    }
    else 
    {
        printk("Wrong frame rate setting \n");
		return FALSE;
    }   
	SIV121DUSetPage(2);
    SIV121DUWriteCmosSensor(0x40,0x40);//Max Analog Gain Value @ Shutter step = Max Shutter step  0x7D
	SIV121DUCalFps();	  
    SIV121DUSetFrameCount();    
    return TRUE;
}

UINT32 SIV121DUFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
	
	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=VGA_PERIOD_PIXEL_NUMS;
        *pFeatureReturnPara16=VGA_PERIOD_LINE_NUMS;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=(VGA_PERIOD_PIXEL_NUMS)+SIV121DUCurrentStatus.iHblank;
        *pFeatureReturnPara16=(VGA_PERIOD_LINE_NUMS)+SIV121DUCurrentStatus.iVblank;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*pFeatureReturnPara32 = SIV121DUCurrentStatus.iPclk;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_ESHUTTER:
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			SIV121DUNightMode((BOOL) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_GAIN:
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
		case SENSOR_FEATURE_SET_REGISTER:
			SIV121DUWriteCmosSensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = SIV121DUReadCmosSensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &SIV121DUSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_YUV_CMD:
//		       printk("SIV121DU YUV sensor Setting:%d, %d \n", *pFeatureData32,  *(pFeatureData32+1));
			SIV121DUYUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		       SIV121DUYUVSetVideoMode(*pFeatureData16);
		       break; 
  case SENSOR_FEATURE_CHECK_SENSOR_ID:
	  SIV121DUGetSensorID(pFeatureReturnPara32); 
	  break; 
		default:
			break;			
	}
	return ERROR_NONE;
}	/* SIV121DUFeatureControl() */

SENSOR_FUNCTION_STRUCT	SensorFuncSIV121DU=
{
	SIV121DUOpen,
	SIV121DUGetInfo,
	SIV121DUGetResolution,
	SIV121DUFeatureControl,
	SIV121DUControl,
	SIV121DUClose
};

UINT32 SIV121DU_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncSIV121DU;

	return ERROR_NONE;
}	/* SensorInit() */
