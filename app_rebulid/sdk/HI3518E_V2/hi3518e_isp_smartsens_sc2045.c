#include "sdk/sdk_debug.h"
#include "hi3518e.h"
#include "hi3518e_isp_sensor.h"
#include "hi_isp_i2c.h"
#include "sdk/sdk_sys.h"

#define SENSOR_NAME "sc2045"
static SENSOR_DO_I2CRD sensor_i2c_read = NULL;
static SENSOR_DO_I2CWR sensor_i2c_write = NULL;

#define SENSOR_I2C_READ(_add, _ret_data) \
	(sensor_i2c_read ? sensor_i2c_read((_add), (_ret_data)) : _i2c_read((_add), (_ret_data), 0x60, 2, 1))

#define SENSOR_I2C_WRITE(_add, _data) \
	(sensor_i2c_write ? sensor_i2c_write((_add), (_data)) : _i2c_write((_add), (_data), 0x60, 2, 1))

#define SENSOR_SC2045_WIDTH 1920
#define SENSOR_SC2045_HEIGHT 1080
#define SC2045_CHECK_DATA_LSB	(0x20)
#define SC2045_CHECK_DATA_MSB	(0x45)//CHIP_ID address is 0x3107&0x3108
#define SENSOR_DELAY_MS(_ms) do{ usleep(1024 * (_ms)); } while(0)

#if !defined(__sc2045_CMOS_H_)
#define __sc2045_CMOS_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hi_comm_sns.h"
#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "mpi_af.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define sc2045_ID 2045

#define CMOS_sc2045_ISP_WRITE_SENSOR_ENABLE (1)


/* To change the mode of config. ifndef INIFILE_CONFIG_MODE, quick config mode.*/
/* else, cmos_cfg.ini file config mode*/
#ifdef INIFILE_CONFIG_MODE

extern AE_SENSOR_DEFAULT_S  g_AeDft[];
extern AWB_SENSOR_DEFAULT_S g_AwbDft[];
extern ISP_CMOS_DEFAULT_S   g_IspDft[];
extern HI_S32 Cmos_LoadINIPara(const HI_CHAR *pcName);
#else

#endif

/****************************************************************************
 * local variables                                                            *
 ****************************************************************************/

static const unsigned int sensor_i2c_addr=0x60;
static unsigned int sensor_addr_byte=2;
static unsigned int sensor_data_byte=1;

#define VMAX_ADDR_H              (0x320e)
#define VMAX_ADDR_L              (0x320f)

#define SENSOR_1080P_30FPS_MODE  (1)

#define INCREASE_LINES (0) /* make real fps less than stand fps because NVR require*/
#define VMAX_1080P30_LINEAR     (1125+INCREASE_LINES)
#define CMOS_OV2035_SLOW_FRAMERATE_MODE (0)

#define FULL_LINES_MAX  (0xFFFF)


static HI_U8 gu8SensorImageMode = SENSOR_1080P_30FPS_MODE;
static WDR_MODE_E genSensorMode = WDR_MODE_NONE;

static HI_U32 gu32FullLinesStd = VMAX_1080P30_LINEAR;
static HI_U32 gu32FullLines = VMAX_1080P30_LINEAR;

static HI_BOOL bInit = HI_FALSE;
static HI_BOOL bSensorInit = HI_FALSE;
static ISP_SNS_REGS_INFO_S g_stSnsRegsInfo = {0};
static ISP_SNS_REGS_INFO_S g_stPreSnsRegsInfo = {0};
static HI_U8 gu8Fps = 30;


/* AE default parameter and function */
static HI_S32 sc2045_get_ae_default(AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    if (HI_NULL == pstAeSnsDft)
    {
        printf("null pointer when get ae default value!\n");
        return -1;
    }

    pstAeSnsDft->u32LinesPer500ms = gu32FullLinesStd*30/2;
    pstAeSnsDft->u32FullLinesStd = gu32FullLinesStd;
    pstAeSnsDft->u32FlickerFreq = 0;
	pstAeSnsDft->u32FullLinesMax=FULL_LINES_MAX;

    pstAeSnsDft->au8HistThresh[0] = 0xd;
    pstAeSnsDft->au8HistThresh[1] = 0x28;
    pstAeSnsDft->au8HistThresh[2] = 0x60;
    pstAeSnsDft->au8HistThresh[3] = 0x80;
            
    pstAeSnsDft->u8AeCompensation = 0x2b;

    pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;
    pstAeSnsDft->stIntTimeAccu.f32Offset = 0;
    pstAeSnsDft->u32MaxIntTime = gu32FullLinesStd - 4;
    pstAeSnsDft->u32MinIntTime = 1;
    pstAeSnsDft->u32MaxIntTimeTarget = pstAeSnsDft->u32MaxIntTime;
    pstAeSnsDft->u32MinIntTimeTarget = pstAeSnsDft->u32MinIntTime;

    pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stAgainAccu.f32Accuracy = 0.0625; 
    pstAeSnsDft->u32MaxAgain = 248;  //15.6//62�� 
    pstAeSnsDft->u32MinAgain = 16;
    pstAeSnsDft->u32MaxAgainTarget = 248;
    pstAeSnsDft->u32MinAgainTarget = 16;

    pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stDgainAccu.f32Accuracy = 0.0625;//invalidate
    pstAeSnsDft->u32MaxDgain = 16;  
    pstAeSnsDft->u32MinDgain = 16;
    pstAeSnsDft->u32MaxDgainTarget = 16;
    pstAeSnsDft->u32MinDgainTarget = 16; 
	
    pstAeSnsDft->u32ISPDgainShift = 8;
    pstAeSnsDft->u32MinISPDgainTarget = 1 << pstAeSnsDft->u32ISPDgainShift;
    pstAeSnsDft->u32MaxISPDgainTarget = 16 << pstAeSnsDft->u32ISPDgainShift; 
	  
    pstAeSnsDft->u32LinesPer500ms = gu32FullLinesStd*30/2;

    return 0;
}

/* the function of sensor set fps */
static HI_VOID sc2045_fps_set(HI_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
   // printf("*******************__FUNCTION__ %s,__LINE__%d***\n",__FUNCTION__,__LINE__);

    HI_U32 u32VblankingLines = 0xFFFF;
    if ((f32Fps <= 30) && (f32Fps >= 0.5))
    {
        if(SENSOR_1080P_30FPS_MODE == gu8SensorImageMode)
        {
            u32VblankingLines = VMAX_1080P30_LINEAR * 30 / f32Fps;
        }
    }
    else
    {
        printf("Not support Fps: %f\n", f32Fps);
        return;
    }

	gu32FullLinesStd = gu32FullLinesStd > FULL_LINES_MAX ? FULL_LINES_MAX : gu32FullLinesStd;

#if CMOS_sc2045_ISP_WRITE_SENSOR_ENABLE
    g_stSnsRegsInfo.astI2cData[4].u32Data = (u32VblankingLines >> 8) & 0xFF ;
	g_stSnsRegsInfo.astI2cData[5].u32Data = u32VblankingLines & 0xFF;
#else
    SENSOR_I2C_WRITE(0x320e, (u32VblankingLines >> 8) & 0xff) ;
    SENSOR_I2C_WRITE(0x320f, u32VblankingLines & 0xff);
#endif

    pstAeSnsDft->f32Fps = f32Fps;
//	printf("%s %d u32MaxIntTime=%d \n",__FUNCTION__,__LINE__,pstAeSnsDft->u32MaxIntTime);
    pstAeSnsDft->u32MaxIntTime = u32VblankingLines - 4;
    pstAeSnsDft->u32MaxIntTimeTarget=u32VblankingLines - 4;
//	printf("%s %d u32MaxIntTime=%d \n",__FUNCTION__,__LINE__,pstAeSnsDft->u32MaxIntTime);
    gu32FullLinesStd = u32VblankingLines;
    gu8Fps = f32Fps;
    pstAeSnsDft->u32LinesPer500ms = gu32FullLinesStd * gu8Fps/ 2;
    pstAeSnsDft->u32FullLinesStd = gu32FullLinesStd;
	
	//gu32FullLines = gu32FullLinesStd;
	//pstAeSnsDft->u32FullLines = gu32FullLines;
#if 1
//printf("**********************%#x,%#x,\n",u32VblankingLines,u32VblankingLines - 0x200);
	SENSOR_I2C_WRITE(0x336a, ((u32VblankingLines >> 8) & 0xFF));
	SENSOR_I2C_WRITE(0x336b, u32VblankingLines & 0xFf);

	SENSOR_I2C_WRITE(0x3368, (((u32VblankingLines - 0x200) >> 8) & 0xFF));
	SENSOR_I2C_WRITE(0x3369, (u32VblankingLines - 0x200) & 0xFf);
#endif
    return;
}


static HI_VOID sc2045_slow_framerate_set(HI_U32 u32FullLines,
    AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{

  //  printf("*******************__FUNCTION__ %s,__LINE__%d***\n",__FUNCTION__,__LINE__);
    u32FullLines = (u32FullLines > 0xFFFF) ? 0xFFFF : u32FullLines;
    gu32FullLines = u32FullLines;
	pstAeSnsDft->u32FullLines = gu32FullLines;//gu32FullLinesStd;
	gu32FullLinesStd = gu32FullLinesStd > FULL_LINES_MAX ? FULL_LINES_MAX : gu32FullLinesStd;


#if CMOS_sc2045_ISP_WRITE_SENSOR_ENABLE
	g_stSnsRegsInfo.astI2cData[4].u32Data = (u32FullLines >> 8) & 0xFF;
	g_stSnsRegsInfo.astI2cData[5].u32Data = u32FullLines & 0xFf;
#else
	SENSOR_I2C_WRITE(0x320e, ((u32FullLines >> 8) & 0xFF));
	SENSOR_I2C_WRITE(0x320f, u32FullLines & 0xFf);
#endif
//	printf("%s %d u32MaxIntTime=%d \n",__FUNCTION__,__LINE__,pstAeSnsDft->u32MaxIntTime);
    pstAeSnsDft->u32MaxIntTime = gu32FullLines - 4;
    pstAeSnsDft->u32MaxIntTimeTarget=gu32FullLines - 4;
//	printf("%s %d u32MaxIntTime=%d \n",__FUNCTION__,__LINE__,pstAeSnsDft->u32MaxIntTime);

#if 1
//printf("***************************u32FullLines %#x\n",u32FullLines);
		SENSOR_I2C_WRITE(0x336a, ((u32FullLines >> 8) & 0xFF));
		SENSOR_I2C_WRITE(0x336b, u32FullLines & 0xFf);
	
		SENSOR_I2C_WRITE(0x3368, (((u32FullLines - 0x200) >> 8) & 0xFF));
		SENSOR_I2C_WRITE(0x3369, (u32FullLines - 0x200) & 0xFF);
#endif
	//pstAeSnsDft->u32LinesPer500ms = gu32FullLinesStd * gu8Fps/ 2;
	//pstAeSnsDft->u32FullLinesStd = gu32FullLinesStd;

    return;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
static HI_U32 u32OldIntTime = 0;
static HI_VOID sc2045_inttime_update(HI_U32 u32IntTime)
{
	//printf("u32IntTime:%#x...........%#x........%#x..\n",u32IntTime, (u32IntTime >> 4) & 0xFF,(u32IntTime<<4) & 0xF0);

#if 1
if (u32OldIntTime != u32IntTime)
{
	//printf("u32IntTime:%#x...........%#x........%#x..\n",u32IntTime, (u32IntTime >> 4) & 0xFF,(u32IntTime<<4) & 0xF0);
	u32OldIntTime = u32IntTime;
}
#endif

#if CMOS_sc2045_ISP_WRITE_SENSOR_ENABLE
    g_stSnsRegsInfo.astI2cData[0].u32Data = (u32IntTime >> 4) & 0xFF; 
    g_stSnsRegsInfo.astI2cData[1].u32Data = (u32IntTime<<4) & 0xF0;
#else
	SENSOR_I2C_WRITE(0x3e01, ((u32IntTime >> 4) & 0xFF));
	SENSOR_I2C_WRITE(0x3e02, (u32IntTime<<4) & 0xF0);
#endif

    return;
}


static HI_VOID sc2045_Again_limit(HI_U32 *pu32Again )
{
		//�������ģ�������߼�?
		HI_U32 u32CurrVal = 0;
		HI_U16  u8HighRegVal = 0,u8LowRegVal = 0;
		static HI_U8 u8CurrFlag = 0;
		static HI_U8 u8StaticAgain = 0xF8;

        #define MAXTEMP 0x1980
        #define MINTEMP 0x1680
 

		if (pu32Again == NULL)
		{
			return;
		}	

		SENSOR_I2C_READ(0x3911,&u8HighRegVal);
		SENSOR_I2C_READ(0x3912,&u8LowRegVal);
		u32CurrVal = u8HighRegVal<<8 | u8LowRegVal;
		
		
		//if ((u32CurrVal >= MAXTEMP) && (u8StaticAgain > 0x20))
		if ((u32CurrVal >= MAXTEMP) && (u8StaticAgain > 0x10))
		{
			 u8StaticAgain -= 1;
			 u8CurrFlag = 1;
		}
		else if ((u32CurrVal <= MINTEMP) && (u8StaticAgain < 0xf8) && (u8CurrFlag == 1))
		{
			 u8StaticAgain += 1;
		}
		else if((u32CurrVal >= MINTEMP) && (u32CurrVal <= MAXTEMP) && (u8CurrFlag == 1))
		{
		    *pu32Again = u8StaticAgain;
		}
		else
		{
			//do nothing
		}

		if ((u32CurrVal <= MINTEMP) && (*pu32Again <= u8StaticAgain) && (u8CurrFlag == 1))//----
  	    {
  	    
  		     u8CurrFlag = 0; 
  	    }


#if 0
        if ((u32CurrVal >= 0x1980) && (u8CurrFlag == 1))
        {
             if(*pu32Again > u8StaticAgain)
             {
                 u8StaticAgain = *pu32Again;
             }
             else
             {
                 *pu32Again = u8StaticAgain;
             }
        }
        if ((u32CurrVal <= 0x1680) && (u8CurrFlag == 1))
        {
             if(*pu32Again > u8StaticAgain)
             {
                 *pu32Again = u8StaticAgain;
             }
        }
        
#else
        if ((u32CurrVal >= MAXTEMP) && (u8CurrFlag == 1))
        {
        
       		 *pu32Again=(u8StaticAgain<*pu32Again)?u8StaticAgain:(*pu32Again);
        }
        if ((u32CurrVal <= MINTEMP) && (u8CurrFlag == 1))
		{
		    
		    *pu32Again=(u8StaticAgain<*pu32Again)?u8StaticAgain:(*pu32Again);
        }
#endif
		return;
}

static HI_VOID sc2045_gains_update(HI_U32 u32Again, HI_U32 u32Dgain)
{
 // 20151222
	//printf("%s......%d......%#x.....%#x......%d.....%#x.....\n",__FUNCTION__,__LINE__, u32Again, u32Dgain, u32TotalAgain, u32RegValul);
	//printf("******u32Again:%d******\n",u32Again);

	//�������ģ�������߼�?
	sc2045_Again_limit(&u32Again);
	
#if CMOS_sc2045_ISP_WRITE_SENSOR_ENABLE
    g_stSnsRegsInfo.astI2cData[2].u32Data = (u32Again & 0xF00) >> 8;
	g_stSnsRegsInfo.astI2cData[3].u32Data = u32Again & 0xFF;
#else
    SENSOR_I2C_WRITE(0x3e08, (u32Again & 0xF00) >> 8);
    SENSOR_I2C_WRITE(0x3e09, u32Again & 0xFF);
#endif	
#if 1	//2016.05.04 noise logic
		if(u32Again <= 0x20)
			SENSOR_I2C_WRITE(0x3630, 0xac);
		else
			SENSOR_I2C_WRITE(0x3630, 0x84);
	
		if(u32Again <= 0x40)
			SENSOR_I2C_WRITE(0x3635, 0xe8);//2016.05.20
		else if(u32Again > 0x40&&u32Again<=0x100)
			SENSOR_I2C_WRITE(0x3635, 0xe4);//2016.05.20
		else 
			SENSOR_I2C_WRITE(0x3635, 0xe0);
#endif

    return;
}


HI_S32 sc2045_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_ae_default    = sc2045_get_ae_default;
    pstExpFuncs->pfn_cmos_fps_set           = sc2045_fps_set;
    pstExpFuncs->pfn_cmos_slow_framerate_set= sc2045_slow_framerate_set;    
    pstExpFuncs->pfn_cmos_inttime_update    = sc2045_inttime_update;
    pstExpFuncs->pfn_cmos_gains_update      = sc2045_gains_update;
    pstExpFuncs->pfn_cmos_again_calc_table  = NULL;//cmos_again_calc_table;
    pstExpFuncs->pfn_cmos_dgain_calc_table  = NULL;//cmos_dgain_calc_table;

    return 0;
}


/* AWB default parameter and function */
static AWB_CCM_S g_stAwbCcm =
{  
	4850,	 
	{		
		0x267,0x80F1,0x8076,	
		0x805E,0x222,0x80C4,	
		0x806F,0x8112,0x281,
		
	},	

	3160,	 
	{		
		0x214,0x8097,0x807D,		
		0x8078,0x20A,0x8092,		
		0x8056,0x811C,0x272,

	},

	2470,	 
	{			 
		0x1FE,0x8058,0x80A6,		
		0x8051,0x1B8,0x8067,		
		0x808E,0x824D,0x3DB,
	} 

};

static AWB_AGC_TABLE_S g_stAwbAgcTable =
{
    /* bvalid */
    1,
	
    /*1,  2,  4,  8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768*/
    /* saturation */   
    //{0x7a,0x7a,0x78,0x74,0x68,0x60,0x58,0x50,0x48,0x40,0x38,0x38,0x38,0x38,0x38,0x38}
    {0x8a,0x8a,0x88,0x84,0x78,0x70,0x68,0x60,0x58,0x50,0x48,0x48,0x48,0x48,0x48,0x48}

};

static HI_S32 sc2045_get_awb_default(AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
    if (HI_NULL == pstAwbSnsDft)
    {
        printf("null pointer when get awb default value!\n");
        return -1;
    }

    memset(pstAwbSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));

    pstAwbSnsDft->u16WbRefTemp = 4850;
    pstAwbSnsDft->au16GainOffset[0] = 368;    
    pstAwbSnsDft->au16GainOffset[1] = 256;    
    pstAwbSnsDft->au16GainOffset[2] = 256;    
    pstAwbSnsDft->au16GainOffset[3] = 333;    
    pstAwbSnsDft->as32WbPara[0] = 143;    
    pstAwbSnsDft->as32WbPara[1] = -48;    
    pstAwbSnsDft->as32WbPara[2] = -159;    
    pstAwbSnsDft->as32WbPara[3] = 153256;    
    pstAwbSnsDft->as32WbPara[4] = 128;    
    pstAwbSnsDft->as32WbPara[5] = -119000;
    
    memcpy(&pstAwbSnsDft->stCcm, &g_stAwbCcm, sizeof(AWB_CCM_S));
    memcpy(&pstAwbSnsDft->stAgcTbl, &g_stAwbAgcTable, sizeof(AWB_AGC_TABLE_S));
    
    return 0;
}

HI_S32 sc2045_init_awb_exp_function(AWB_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    memset(pstExpFuncs, 0, sizeof(AWB_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_awb_default = sc2045_get_awb_default;

    return 0;
}

#define DMNR_CALIB_CARVE_NUM_sc2045 7

float g_coef_calib_sc2045[DMNR_CALIB_CARVE_NUM_sc2045][4] = 
{
    {100.000000f, 2.000000f, 0.036754f, 9.532179f, }, 

    {206.000000f, 2.313867f, 0.039446f, 9.782226f, }, 

    {487.000000f, 2.687529f, 0.043005f, 11.625935f, }, 

    {868.000000f, 2.938520f, 0.049732f, 12.633399f, }, 

    {1225.000000f, 3.088136f, 0.053179f, 13.770162f, }, 

    {2607.000000f, 3.416141f, 0.057038f, 16.686234f, }, 

    {3250.000000f, 3.511883f, 0.048118f, 30.336805f, },

};


static ISP_NR_ISO_PARA_TABLE_S g_stNrIsoParaTab[HI_ISP_NR_ISO_LEVEL_MAX] = 
{
     //u16Threshold//u8varStrength//u8fixStrength//u8LowFreqSlope	
       {1500,       160,             256-256,            0 },  //100    //                      //                                                
       {1500,       120,             256-256,            0 },  //200    // ISO                  // ISO //u8LowFreqSlope
       {1500,       100,             256-256,            0 },  //400    //{400,  1200, 96,256}, //{400 , 0  }
       {1750,       80,              256-256,            8 },  //800    //{800,  1400, 80,256}, //{600 , 2  }
       {1500,       255,             256-256,            6 },  //1600   //{1600, 1200, 72,256}, //{800 , 8  }
       {1500,       255,             256-256,            0 },  //3200   //{3200, 1200, 64,256}, //{1000, 12 }
       {1375,       255,             256-256,            0 },  //6400   //{6400, 1100, 56,256}, //{1600, 6  }
       {1375,       255,             256-256,            0 },  //12800  //{12000,1100, 48,256}, //{2400, 0  }
       {1375,       255,             256-256,            0 },  //25600  //{36000,1100, 48,256}, //
       {1375,       255,             256-256,            0 },  //51200  //{64000,1100, 96,256}, //
       {1250,       255,             256-256,            0 },  //102400 //{82000,1000,240,256}, //
       {1250,       255,             256-256,            0 },  //204800 //                           //
       {1250,       255,             256-256,            0 },  //409600 //                           //
       {1250,       255,             256-256,            0 },  //819200 //                           //
       {1250,       255,             256-256,            0 },  //1638400//                           //
       {1250,       255,             256-256,            0 },  //3276800//                           //
};

static ISP_CMOS_DEMOSAIC_S g_stIspDemosaic =
{
	/*For Demosaic*/
	1, /*bEnable*/			
	24,/*u16VhLimit*/	
	40-24,/*u16VhOffset*/
	24,   /*u16VhSlope*/
	/*False Color*/
	1,    /*bFcrEnable*/
	{ 8, 8, 8, 8, 8, 8, 8, 8, 3, 0, 0, 0, 0, 0, 0, 0},    /*au8FcrStrength[ISP_AUTO_ISO_STENGTH_NUM]*/
	{24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24},    /*au8FcrThreshold[ISP_AUTO_ISO_STENGTH_NUM]*/
	/*For Ahd*/
	400, /*u16UuSlope*/	
	{512,512,512,512,512,512,512,  400,  0,0,0,0,0,0,0,0}    /*au16NpOffset[ISP_AUTO_ISO_STENGTH_NUM]*/
};

static ISP_CMOS_GE_S g_stIspGe =
{
	/*For GE*/
	0,    /*bEnable 1  */	
	7,    /*u8Slope*/	
	7,    /*u8Sensitivity*/
	4096, /*u16Threshold  8192 */
	4096, /*u16SensiThreshold   8192*/	
	{1024,1024,1024,2048,2048,2048,2048,  2048,  2048,2048,2048,2048,2048,2048,2048,2048}    /*au16Strength[ISP_AUTO_ISO_STENGTH_NUM]*/	
};

static ISP_CMOS_RGBSHARPEN_S g_stIspRgbSharpen =
{      
  //{100,200,400,800,1600,3200,6400,12800,25600,51200,102400,204800,409600,819200,1638400,3276800};
    {0,	  0,   0,   0,    0,   1,   1,   1,    1,    1,    1,     1,     1,     1,     1,       1},/* enPixSel = ~bEnLowLumaShoot */
        
    {30,  24,  20,  18,  18,  18,  16,  16,    16,  16,  250,   250,   250,   250,    250,    250},/*maxSharpAmt1 = SharpenUD*16 */
    {32, 28, 24, 20,  20,  20,  18,  18,    18,  18,  18,   250,   250,   250,    250,    250},/*maxEdgeAmt = SharpenD*16 */

    //{120,  64,  64,  43,  43,  43,  18,  18,    18,  200,  250,   250,   250,   250,    250,    250},/*maxSharpAmt1 = SharpenUD*16 */  /*9712*/
    //{128, 200, 103, 86,  86,  86,  80,  80,    80,  250,  250,   250,   250,   250,    250,    250},/*maxEdgeAmt = SharpenD*16 */   /*9712*/
       
    {0,   0,   0,    0,   0,   0,   0,   40,  190,  200,  220,   250,   250,   250,     250,       250},/*sharpThd2 = TextureNoiseThd*4 */
    {0,   0,   0,    0,   0,   0,   0,   60,  140,    0,    0,     0,    0,     0,     0,       0},/*edgeThd2 = EdgeNoiseThd*4 */
    {59,  59,  59,  59,  59,  59,  59,   59,  101,  0,   0,    0,   0,    0,    0,      0},/*overshootAmt*/
    {59, 59, 59, 59, 59, 59, 59,  59, 59,  0,   0,    0,   0,    0,    0,     0},/*undershootAmt*/
};


static ISP_CMOS_UVNR_S g_stIspUVNR = 
{
	/*��ֵ�˲��л���UVNR��ISO��ֵ*/
	/*UVNR�л�����ֵ�˲���ISO��ֵ*/
    /*0.0   -> disable��(0.0, 1.0]  -> weak��(1.0, 2.0]  -> normal��(2.0, 10.0) -> strong*/
	/*��˹�˲����ı�׼��*/
  //{100,	200,	400,	800,	1600,	3200,	6400,	12800,	25600,	51200,	102400,	204800,	409600,	819200,	1638400,	3276800};
	{1,	    2,       4,      5,      7,      48,     32,     16,     16,     16,      16,     16,     16,     16,     16,        16},      /*UVNRThreshold*/
 	{0,		0,		0,		0,		0,		0,		0,		0,		0,		1,			1,		2,		2,		2,		2,		2},  /*Coring_lutLimit*/
	{0,		0,		0,		16,		34,		34,		34,		34,		34,		34,		34,		34,		34,		34,		34,			34}  /*UVNR_blendRatio*/
};

static ISP_CMOS_DPC_S g_stCmosDpc = 
{
	//1,/*IR_channel*/
	//1,/*IR_position*/
	//{0,0,0,1,1,1,2,2,2,3,3,3,3,3,3,3},/*au16Strength[16]*/
	{70,100,180,244,248,250,252,252,252,252,252,252,252,252,252,252},/*au16Strength[16]*/
	{0,0,0,0,0,0,0,0,0x24,0x80,0x80,0x80,0xE5,0xE5,0xE5,0xE5},/*au16BlendRatio[16]*/
};


static ISP_CMOS_DRC_S g_stIspDrc =
{
    0,
    10,
    0,
    2,
    192,
    60,
    0,
    0,
    0,
    {1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024}
};

/*static ISP_CMOS_RGBIR_S g_stCmosRgbir =
{
    {
        1,
        1,
        ISP_IRPOS_TYPE_GR,
        0x41,
        4095,
    },
    {
        0,
        1,
        1,
        OP_TYPE_AUTO,
        0x100,
        {269,7,27,-27,293,13,-49,42,255,-295,-271,-253,-37,-125,-60}
    }
};
*/
HI_U32 sc2045_get_isp_default(ISP_CMOS_DEFAULT_S *pstDef)
{
    if (HI_NULL == pstDef)
    {
        printf("null pointer when get isp default value!\n");
        return -1;
    }

    memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));
/*    pstDef->stDrc.bEnable               = HI_FALSE;
    pstDef->stDrc.u8Asymmetry           = 0x02;
    pstDef->stDrc.u8SecondPole          = 0xC0;
    pstDef->stDrc.u8Stretch             = 0x3C;
    pstDef->stDrc.u16BrightGainLmt      = 0x7F;
    pstDef->stDrc.u16DarkGainLmtC       = 0x7F;
    pstDef->stDrc.u16DarkGainLmtY       = 0x7F;
    pstDef->stDrc.u8RangeVar            = 0x00;
    pstDef->stDrc.u8SpatialVar          = 0x0A;
*/    


 //   memcpy(&pstDef->stLsc.stLscParaTable[0], &g_stCmosLscTable[0], sizeof(ISP_LSC_CABLI_TABLE_S)*HI_ISP_LSC_LIGHT_NUM);  
 	memcpy(&pstDef->stDrc, &g_stIspDrc, sizeof(ISP_CMOS_DRC_S));
    memcpy(&pstDef->stDemosaic, &g_stIspDemosaic, sizeof(ISP_CMOS_DEMOSAIC_S));
    memcpy(&pstDef->stRgbSharpen, &g_stIspRgbSharpen, sizeof(ISP_CMOS_RGBSHARPEN_S));
    memcpy(&pstDef->stGe, &g_stIspGe, sizeof(ISP_CMOS_GE_S));			
  //  pstDef->stNoiseTbl.u8SensorIndex = HI_ISP_NR_SENSOR_INDEX_sc2045;
    pstDef->stNoiseTbl.stNrCaliPara.u8CalicoefRow = DMNR_CALIB_CARVE_NUM_sc2045;
    pstDef->stNoiseTbl.stNrCaliPara.pCalibcoef    = (HI_FLOAT (*)[4])g_coef_calib_sc2045;

    memcpy(&pstDef->stNoiseTbl.stIsoParaTable[0], &g_stNrIsoParaTab[0],sizeof(ISP_NR_ISO_PARA_TABLE_S)*HI_ISP_NR_ISO_LEVEL_MAX);

    memcpy(&pstDef->stUvnr,       &g_stIspUVNR,       sizeof(ISP_CMOS_UVNR_S));
    memcpy(&pstDef->stDpc,       &g_stCmosDpc,       sizeof(ISP_CMOS_DPC_S));
   // memcpy(&pstDef->stRgbir,       &g_stCmosRgbir,       sizeof(ISP_CMOS_RGBIR_S));
 //   memcpy(&pstDef->stLsc,       &g_stCmosLscTable,       sizeof(ISP_LSC_CABLI_TABLE_S));

    pstDef->stSensorMaxResolution.u32MaxWidth  = SENSOR_SC2045_WIDTH;
    pstDef->stSensorMaxResolution.u32MaxHeight = SENSOR_SC2045_HEIGHT;

    return 0;
}

HI_U32 sc2045_get_isp_black_level(ISP_CMOS_BLACK_LEVEL_S *pstBlackLevel)
{
   // HI_S32  i;
    
    if (HI_NULL == pstBlackLevel)
    {
        printf("null pointer when get isp black level value!\n");
        return -1;
    }

    /* Don't need to update black level when iso change */
    pstBlackLevel->bUpdate = HI_FALSE;
          
    pstBlackLevel->au16BlackLevel[0] = 70;
    pstBlackLevel->au16BlackLevel[1] = 70;
    pstBlackLevel->au16BlackLevel[2] = 70;
    pstBlackLevel->au16BlackLevel[3] = 70;

    return 0;  
    
}

HI_VOID sc2045_set_pixel_detect(HI_BOOL bEnable)
{
	HI_U32 u32Lines = VMAX_1080P30_LINEAR * 30 /5;
	
#if CMOS_sc2045_ISP_WRITE_SENSOR_ENABLE
    if (bEnable) /* setup for ISP pixel calibration mode */
    {
        /* 5 fps */
		SENSOR_I2C_WRITE(0x320e, (u32Lines >> 4) && 0xFF);
		SENSOR_I2C_WRITE(0x320f, ((u32Lines<<4)&&0xF0));
       
        /* max exposure time*/
		

    }
    else /* setup for ISP 'normal mode' */
    { 
        SENSOR_I2C_WRITE(0x320e, (gu32FullLinesStd >> 8) && 0XFF);
        SENSOR_I2C_WRITE(0x320f, gu32FullLinesStd && 0xFF);
        
        bInit = HI_FALSE;
    }
#else
    if (bEnable) /* setup for ISP pixel calibration mode */
    {
        
		SENSOR_I2C_WRITE(0x3e01, (u32Lines >> 8) && 0xFF);
		SENSOR_I2C_WRITE(0x3e02, (u32Lines - 4) && 0xFF);
        
        /* min gain */
        SENSOR_I2C_WRITE(0x3e0e, 0x00);
		SENSOR_I2C_WRITE(0x3e0f, 0x00);

		/* 5 fps */
        SENSOR_I2C_WRITE(0x320e, (u32Lines >> 8) && 0xFF);
        SENSOR_I2C_WRITE(0x320f, u32Lines && 0xFF);
    }
    else /* setup for ISP 'normal mode' */
    { 
        SENSOR_I2C_WRITE(0x320e, (gu32FullLinesStd >> 8) && 0XFF);
        SENSOR_I2C_WRITE(0x320f, gu32FullLinesStd && 0xFF);
        
        bInit = HI_FALSE;
    }
#endif

    return;
}

HI_VOID sc2045_set_wdr_mode(HI_U8 u8Mode)
{
    bInit = HI_FALSE;
    
    switch(u8Mode)
    {
        case WDR_MODE_NONE:
            if (SENSOR_1080P_30FPS_MODE == gu8SensorImageMode)
            {
                gu32FullLinesStd = VMAX_1080P30_LINEAR;
            }
            genSensorMode = WDR_MODE_NONE;
            printf("linear mode\n");
        break;
        default:
            printf("NOT support this mode!\n");
            return;
        break;
    }
    return;
}

HI_U32 sc2045_get_sns_regs_info(ISP_SNS_REGS_INFO_S *pstSnsRegsInfo)
{

#if CMOS_sc2045_ISP_WRITE_SENSOR_ENABLE

    HI_S32 i;

    if (HI_FALSE == bInit)
    {
        g_stSnsRegsInfo.enSnsType = ISP_SNS_I2C_TYPE;
        g_stSnsRegsInfo.u8Cfg2ValidDelayMax = 2;		
        g_stSnsRegsInfo.u32RegNum = 6;
	
        for (i=0; i<g_stSnsRegsInfo.u32RegNum; i++)
        {	
            g_stSnsRegsInfo.astI2cData[i].bUpdate = HI_TRUE;
            g_stSnsRegsInfo.astI2cData[i].u8DevAddr = sensor_i2c_addr;
            g_stSnsRegsInfo.astI2cData[i].u32AddrByteNum = sensor_addr_byte;
            g_stSnsRegsInfo.astI2cData[i].u32DataByteNum = sensor_data_byte;
        }		
        g_stSnsRegsInfo.astI2cData[0].u8DelayFrmNum = 0;
        g_stSnsRegsInfo.astI2cData[0].u32RegAddr = 0x3e01;     //exp high  bit[7:0] 
        g_stSnsRegsInfo.astI2cData[1].u8DelayFrmNum = 0;
        g_stSnsRegsInfo.astI2cData[1].u32RegAddr = 0x3e02;     //exp low  bit[7:4] 
        g_stSnsRegsInfo.astI2cData[2].u8DelayFrmNum = 0;
        g_stSnsRegsInfo.astI2cData[2].u32RegAddr = 0x3e08;     //digita agin[6:5];    coarse analog again[4:2]
		g_stSnsRegsInfo.astI2cData[3].u8DelayFrmNum = 0;
        g_stSnsRegsInfo.astI2cData[3].u32RegAddr = 0x3e09;     //fine analog again[4:0]

		g_stSnsRegsInfo.astI2cData[4].u8DelayFrmNum = 0;
        g_stSnsRegsInfo.astI2cData[4].u32RegAddr = 0x320e;     //TIMING_VTS  high bit[7:0] 
		g_stSnsRegsInfo.astI2cData[5].u8DelayFrmNum = 0;
        g_stSnsRegsInfo.astI2cData[5].u32RegAddr = 0x320f;     //TIMING_VTS  low bit[7:0] 

	
        bInit = HI_TRUE;
    }
    else    
    {        
        for (i=0; i<g_stSnsRegsInfo.u32RegNum; i++)        
        {            
            if (g_stSnsRegsInfo.astI2cData[i].u32Data == g_stPreSnsRegsInfo.astI2cData[i].u32Data)            
            {                
               // printf("HI_FALSE....\n");
                g_stSnsRegsInfo.astI2cData[i].bUpdate = HI_TRUE;
            }            
            else            
	        {
	             if(i == 0)
	             {
		//			printf("i:%d..g_stSnsRegsInfo:%#x  g_stPreSnsRegsInfo:%#x.....\n",i,g_stSnsRegsInfo.astI2cData[i].u32Data, g_stPreSnsRegsInfo.astI2cData[i].u32Data);
	     //           printf("#####################HI_TRUE....\n");       
	             }
                g_stSnsRegsInfo.astI2cData[i].bUpdate = HI_TRUE;
            }        
        }    
    }

    if (HI_NULL == pstSnsRegsInfo)
    {
        printf("null pointer when get sns reg info!\n");
        return -1;
    }

//printf(".%#X..%#X.\n",g_stSnsRegsInfo.astI2cData[0].u32Data,g_stPreSnsRegsInfo.astI2cData[0].u32Data);
    memcpy(pstSnsRegsInfo, &g_stSnsRegsInfo, sizeof(ISP_SNS_REGS_INFO_S)); 
    memcpy(&g_stPreSnsRegsInfo, &g_stSnsRegsInfo, sizeof(ISP_SNS_REGS_INFO_S)); 
#endif
    return 0;
}

static HI_S32 sc2045_set_image_mode(ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode)
{
    HI_U8 u8SensorImageMode = gu8SensorImageMode;

    bInit = HI_FALSE;
// printf("%s.............................%d....\n",__FUNCTION__,__LINE__);   
    if (HI_NULL == pstSensorImageMode )
    {
        printf("null pointer when set image mode\n");
        return -1;
    }

    if ((pstSensorImageMode->u16Width <= 1920) && (pstSensorImageMode->u16Height <= 1080))
    {
        if (WDR_MODE_NONE == genSensorMode)
        {
            if (pstSensorImageMode->f32Fps <= 30)
            {
                u8SensorImageMode = SENSOR_1080P_30FPS_MODE;
            }
            else
            {
                printf("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n", 
                    pstSensorImageMode->u16Width, 
                    pstSensorImageMode->u16Height,
                    pstSensorImageMode->f32Fps,
                    genSensorMode);

                return -1;
            }
        }
        else
        {
            printf("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n", 
                pstSensorImageMode->u16Width, 
                pstSensorImageMode->u16Height,
                pstSensorImageMode->f32Fps,
                genSensorMode);

            return -1;
        }
    }
    else
    {
        printf("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n", 
            pstSensorImageMode->u16Width, 
            pstSensorImageMode->u16Height,
            pstSensorImageMode->f32Fps,
            genSensorMode);

        return -1;
    }

    /* Sensor first init */
    if (HI_FALSE == bSensorInit)
    {
        gu8SensorImageMode = u8SensorImageMode;
        
        return 0;
    }

    /* Switch SensorImageMode */
    if (u8SensorImageMode == gu8SensorImageMode)
    {
        /* Don't need to switch SensorImageMode */
        return -1;
    }
    
    gu8SensorImageMode = u8SensorImageMode;
//	printf("%s.............................%d....\n",__FUNCTION__,__LINE__);   

    return 0;
}

HI_VOID sc2045_global_init()
{   
    gu8SensorImageMode = SENSOR_1080P_30FPS_MODE;
    genSensorMode = WDR_MODE_NONE;
    gu32FullLinesStd = VMAX_1080P30_LINEAR; 
    gu32FullLines = VMAX_1080P30_LINEAR;
    bInit = HI_FALSE;
    bSensorInit = HI_FALSE; 

    memset(&g_stSnsRegsInfo, 0, sizeof(ISP_SNS_REGS_INFO_S));
    memset(&g_stPreSnsRegsInfo, 0, sizeof(ISP_SNS_REGS_INFO_S));
}


static void sc2045_reg_init(void)
{	//20160128
	
		SENSOR_I2C_WRITE(0x0100,0x00);  // STOP STREAM
		  
		SENSOR_I2C_WRITE(0x3039,0x31);  
		   
		SENSOR_I2C_WRITE(0x3d08,0x00); 
		//SENSOR_I2C_WRITE(0x320d,0xe8); 
		SENSOR_I2C_WRITE(0x3e14,0x30);  //ramp config
		SENSOR_I2C_WRITE(0x3637,0xbc);//bf
		SENSOR_I2C_WRITE(0x3638,0x84);
		
		SENSOR_I2C_WRITE(0x5000,0x07); //dpc
		SENSOR_I2C_WRITE(0x5001,0x45); 	  
		SENSOR_I2C_WRITE(0x5780,0x15); //manual mode
		SENSOR_I2C_WRITE(0x5782,0x03); //white thres
		SENSOR_I2C_WRITE(0x57a0,0x10);//black thres
	
		SENSOR_I2C_WRITE(0x3300,0x08); //timing
		SENSOR_I2C_WRITE(0x3306,0x38);
		SENSOR_I2C_WRITE(0x3308,0x10); 	  
		SENSOR_I2C_WRITE(0x330b,0x90); 
		SENSOR_I2C_WRITE(0x3367,0x08); //precharge
		SENSOR_I2C_WRITE(0x330e,0x30);//2016.05.20
		SENSOR_I2C_WRITE(0x3334,0x40); //comp all high
	
		SENSOR_I2C_WRITE(0x3e03,0x03); //ae
		SENSOR_I2C_WRITE(0x3e08,0x00);   
		SENSOR_I2C_WRITE(0x3e01,0x40);  
		SENSOR_I2C_WRITE(0x3416,0x10);  
		SENSOR_I2C_WRITE(0x3e0f,0x90);
	
		SENSOR_I2C_WRITE(0x3631,0x80);  //analog
		SENSOR_I2C_WRITE(0x3635,0xe0);
		SENSOR_I2C_WRITE(0x3620,0x82);  
		SENSOR_I2C_WRITE(0x3621,0x28);  
		SENSOR_I2C_WRITE(0x3627,0x03);  
		SENSOR_I2C_WRITE(0x3626,0x02); 
	
		SENSOR_I2C_WRITE(0x3622,0x0e);//blksun//2016.05.20
		SENSOR_I2C_WRITE(0x3630,0xac);  
		SENSOR_I2C_WRITE(0x3633,0x8c);//2016.05.20  
		SENSOR_I2C_WRITE(0x363a,0x14);  
	
		SENSOR_I2C_WRITE(0x303f,0x82); 
		SENSOR_I2C_WRITE(0x3c03,0x08); 
		SENSOR_I2C_WRITE(0x320c,0x03);  // hts=2640=1320x2  
		SENSOR_I2C_WRITE(0x320d,0xe8); 
	
		SENSOR_I2C_WRITE(0x3f00,0x06); //sram write
		SENSOR_I2C_WRITE(0x3f04,0x01);   
		SENSOR_I2C_WRITE(0x3f05,0x48); 
		SENSOR_I2C_WRITE(0x4501,0xa4);
 
		SENSOR_I2C_WRITE(0x3039,0x07); //135M PLL
		SENSOR_I2C_WRITE(0x303a,0x8e); 
		//2016.05.20
		SENSOR_I2C_WRITE(0x3662,0x82);
		SENSOR_I2C_WRITE(0x3631,0x84);   
		SENSOR_I2C_WRITE(0x3633,0x8f); 
		SENSOR_I2C_WRITE(0x3038,0x84);
		
 		//2016.05.20
		SENSOR_I2C_WRITE(0x3312,0x06);
		SENSOR_I2C_WRITE(0x3340,0x03);   
		SENSOR_I2C_WRITE(0x3341,0x68); 
		SENSOR_I2C_WRITE(0x3342,0x01);
		SENSOR_I2C_WRITE(0x3343,0x80);

		SENSOR_I2C_WRITE(0x335d,0x2a);
		SENSOR_I2C_WRITE(0x3348,0x03);   
		SENSOR_I2C_WRITE(0x3349,0x68); 
		SENSOR_I2C_WRITE(0x334a,0x01);
		SENSOR_I2C_WRITE(0x334b,0x80); 

		//2016.06.29 
		SENSOR_I2C_WRITE(0x3620,0x82);
		SENSOR_I2C_WRITE(0x3627,0x02);	 
		SENSOR_I2C_WRITE(0x3341,0x88); 
		SENSOR_I2C_WRITE(0x3349,0x88);
		SENSOR_I2C_WRITE(0x3636,0x8c); 
		SENSOR_I2C_WRITE(0x3639,0x90); 
		SENSOR_I2C_WRITE(0x3334,0x00);
		SENSOR_I2C_WRITE(0x3f01,0x04); 

		SENSOR_I2C_WRITE(0x3416,0x11); 
		SENSOR_I2C_WRITE(0x3307,0xff); 

		SENSOR_I2C_WRITE(0x3637,0xba);
		SENSOR_I2C_WRITE(0x3306,0x46);	 
		SENSOR_I2C_WRITE(0x3343,0x70); 
		SENSOR_I2C_WRITE(0x334b,0x70);
		SENSOR_I2C_WRITE(0x3c00,0x45); 
		SENSOR_I2C_WRITE(0x3c03,0x04); 
		SENSOR_I2C_WRITE(0x3f05,0x20);

		SENSOR_I2C_WRITE(0x0100,0x01); //OUTPUT
	
		bSensorInit = HI_TRUE;
		printf("===sc2045 sensor 1080P30fps(Parallel port) init success!=====\n");

}


HI_S32 sc2045_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));

    pstSensorExpFunc->pfn_cmos_sensor_init = sc2045_reg_init;
    //pstSensorExpFunc->pfn_cmos_sensor_exit = sensor_exit;
    pstSensorExpFunc->pfn_cmos_sensor_global_init = sc2045_global_init;
    pstSensorExpFunc->pfn_cmos_set_image_mode = sc2045_set_image_mode;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode = sc2045_set_wdr_mode;
    
    pstSensorExpFunc->pfn_cmos_get_isp_default = sc2045_get_isp_default;
    pstSensorExpFunc->pfn_cmos_get_isp_black_level = sc2045_get_isp_black_level;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect = sc2045_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_get_sns_reg_info = sc2045_get_sns_regs_info;

    return 0;
}

/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/
 
int sc2045_register_callback(void)
{
    ISP_DEV IspDev = 0;
    HI_S32 s32Ret;
    ALG_LIB_S stLib;
    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;

    sc2045_init_sensor_exp_function(&stIspRegister.stSnsExp);
    s32Ret = HI_MPI_ISP_SensorRegCallBack(IspDev, sc2045_ID, &stIspRegister);
    if (s32Ret)
    {
        printf("sensor register callback function failed!\n");
        return s32Ret;
    }
    
    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    sc2045_init_ae_exp_function(&stAeRegister.stSnsExp);
    s32Ret = HI_MPI_AE_SensorRegCallBack(IspDev, &stLib, sc2045_ID, &stAeRegister);
    if (s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    sc2045_init_awb_exp_function(&stAwbRegister.stSnsExp);
    s32Ret = HI_MPI_AWB_SensorRegCallBack(IspDev, &stLib, sc2045_ID, &stAwbRegister);
    if (s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }
    
    return 0;
}

int sc2045_unregister_callback(void)
{
    ISP_DEV IspDev = 0;
    HI_S32 s32Ret;
    ALG_LIB_S stLib;

    s32Ret = HI_MPI_ISP_SensorUnRegCallBack(IspDev, sc2045_ID);
    if (s32Ret)
    {
        printf("sensor unregister callback function failed!\n");
        return s32Ret;
    }
    
    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    s32Ret = HI_MPI_AE_SensorUnRegCallBack(IspDev, &stLib, sc2045_ID);
    if (s32Ret)
    {
        printf("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    stLib.s32Id = 0;
    strncpy(stLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    s32Ret = HI_MPI_AWB_SensorUnRegCallBack(IspDev, &stLib, sc2045_ID);
    if (s32Ret)
    {
        printf("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }
    
    return 0;
}

int SmartSens_SC2045_init(SENSOR_DO_I2CRD do_i2c_read, 
	SENSOR_DO_I2CWR do_i2c_write)//ISP_AF_REGISTER_S *pAfRegister
{
	//SENSOR_EXP_FUNC_S sensor_exp_func;

	// init i2c buf
	sensor_i2c_read = do_i2c_read;
	sensor_i2c_write = do_i2c_write;

//	ar0130_reg_init();

	sc2045_register_callback();
//	af_register_callback(pAfRegister);

	ALG_LIB_S stLib;
	HI_S32 s32Ret;
    /* 1. register ae lib */
    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AE_LIB_NAME);
    s32Ret = HI_MPI_AE_Register(0,&stLib);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_AE_Register failed!\n", __FUNCTION__);
        return s32Ret;
    }

    /* 2. register awb lib */
    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AWB_LIB_NAME);
    s32Ret = HI_MPI_AWB_Register(0,&stLib);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_AWB_Register failed!\n", __FUNCTION__);
        return s32Ret;
    }

    /* 3. register af lib */
    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AF_LIB_NAME);
    s32Ret = HI_MPI_AF_Register(0,&stLib);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_AF_Register failed!\n", __FUNCTION__);
        return s32Ret;
    }
	printf("SmartSens sc2045 sensor 1080P30fps init success!\n");
	return s32Ret;
}

int SC2045_get_resolution(uint32_t *ret_width, uint32_t *ret_height)
{
    if(ret_width && ret_height){
        *ret_width = SENSOR_SC2045_WIDTH;
        *ret_height = SENSOR_SC2045_HEIGHT;
        return 0;
    }
    return -1;
}

bool SENSOR_SC2045_probe()
{
    uint16_t ret_data1 = 0;
    uint16_t ret_data2 = 0;

    SENSOR_I2C_READ(0x3107, &ret_data1);
    SENSOR_I2C_READ(0x3108, &ret_data2);
	
    if(ret_data1 == SC2045_CHECK_DATA_LSB && ret_data2 == SC2045_CHECK_DATA_MSB){
        //set i2c pinmux
		sdk_sys->write_reg(0x200f0040, 0x2);
	    sdk_sys->write_reg(0x200f0044, 0x2);
		
        sdk_sys->write_reg(0x2003002c, 0xb4001);	 // sensor unreset, clk 27MHz, VI 99MHz

        return true;
    }
    return false;
}
int SC2045_get_sensor_name(char *sensor_name)
{
    if(sensor_name != NULL) {
        memcpy(sensor_name,SENSOR_NAME,strlen(SENSOR_NAME));
		return 0;
    }
    return -1;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __sc2045_CMOS_H_ */



