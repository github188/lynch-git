/*
*	IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
*
*	By downloading, copying, installing or using the software you agree to this license.
*	If you do not agree to this license, do not download, install,
*	Copy or use the software.
*
*	Copyright (C) 2011, ANTS, Inc, All Rights Reserved.
*
*	Project Name:ServerCore
*	File Name:AntsServerSDK.h
*
*	Writed by ItmanLee at 2011 - 02 - 08 Ants,WuHan,HuBei,China
*/
#ifndef __ANTS_SERVER_SDK_H__
#define __ANTS_SERVER_SDK_H__
#include "AntsMidLayerSDK.h"

#ifdef WIN32
#ifdef ANTSSERVERSDK_EXPORTS
#define ANTS_SERVER_API __declspec(dllexport)
#else
#define ANTS_SERVER_API __declspec(dllimport)
#endif
#else
#define __stdcall 
#define CALLBACK
#define ANTS_SERVER_API extern "C"
#endif

typedef enum AntsLogDest{
	AntsLog2File,
	AntsLog2Console,
	AntsLog2Socket
}eANTS_LOG_DEST;

typedef enum {
	eG711A=1,
	eG711U=2,
	eG722=3,
	eG726=4,
	eG723=5,
	eAMRNB=6,
	eAMRWB=7,
	eADPCM=8,
	ePCM=9,
	eADPCMC1=10,
	eG72616K=11,	
}eANTS_AUDIOCODEC_ID;

typedef struct{
	DWORD dwLogDest;
	DWORD dwListenPort;
	DWORD dwMainStreamBufferNum;
	DWORD dwMainStreamBlockSize;
	DWORD dwSubStreamBufferNum;
	DWORD dwSubStreamBlockSize;
	BYTE byRes[128];	
}ANTS_SERVER_PARAM,*LPANTS_SERVER_PARAM;

typedef struct{
	fSetParameter fpSetParameter;//!参数配置获取与设置
	fGetParameter fpGetParameter;
	fPtzControl fpPtzControl;//!PTZ操作
	fPtzPreset fpPtzPreset;
	fPtzTrack fpPtzTrack;
	fPtzCruise fpPtzCruise;
	fPtzTrans fpPtzTrans;
	fPtz3D fpPtz3D;
	fGetPtzCruise fpGetPtzCruise;
	fCaptureJpeg fpCaptureJpeg;//!Jpeg抓图操作
	fGetChannelBitRate fpGetChannelBitRate;//!获取通道实时主码流及子码流信息
	fGetAlarmInfo fpGetAlarmInfo;//查询报警
	fGetAlarmOut fpGetAlarmOut;//!报警输出状态获取与设置
	fSetAlarmOut fpSetAlarmOut;
	fStartVoice fpStartVoice;//!语音对讲操作
	fStopVoice fpStopVoice;
	fSendVoiceData fpSendVoiceData;
	fReboot fpReboot;
	fGetSDKVersion fpGetSDKVersion;//!SDK版本信息获取与错误码获取
	fGetLastError fpGetLastError;
	fCheckPassword fpCheckPassword;
	fMakeIFrame fpMakeIFrame;
	fInputData fpInputData;
	fStartHistoryStream fpStartHistoryStream;
	fControlHistoryStream fpControlHistoryStream;
	fStopHistoryStream fpStopHistoryStream;
	fStartFindFile fpStartFindFile;
	fFindNextFile fpFindNextFile;
	fStopFindFile fpStopFindFile;
	fQueryFileMonthly fpQueryFileMonthly;
	fChangeStream fpChangeStream ;
}ANTS_SERVER_FUNCTION_CONFIG,*LPANTS_SERVER_FUNCTION_CONFIG;

#ifdef __cplusplus
extern "C"{
#endif

BOOL __stdcall ANTS_SERVER_Initialize(LPANTS_SERVER_PARAM lpServerParam,LPANTS_SERVER_FUNCTION_CONFIG lpServerFunctionConfig);
BOOL __stdcall ANTS_SERVER_InputData(DWORD dwStreamType,DWORD dwChannel,BYTE *lpBuffer,DWORD dwSize,BYTE byFrameType);//!音频默认编码类型G711A 默认帧率25  视频默认帧率25
BOOL __stdcall ANTS_SERVER_InputDataV2(DWORD dwStreamType,DWORD dwChannel,BYTE *lpBuffer,DWORD dwSize,BYTE byFrameType,WORD wWidth,WORD wHeight);//!音频默认编码类型G711A 默认帧率25  视频默认帧率25
BOOL __stdcall ANTS_SERVER_InputDataV3(DWORD dwStreamType,DWORD dwChannel,BYTE *lpBuffer,DWORD dwSize,BYTE byFrameType,WORD wWidth,WORD wHeight,DWORD dwSecond,DWORD dwUSecond);
BOOL __stdcall ANTS_SERVER_InputDataV4(DWORD dwStreamType,DWORD dwChannel,BYTE *lpBuffer,DWORD dwSize,BYTE byFrameType,DWORD dwFrameRate);
BOOL __stdcall ANTS_SERVER_InputOneAudioFrame(DWORD dwStreamType,DWORD dwChannel,BYTE *lpBuffer,DWORD dwSize,BYTE byAudioCodecId);//!新增音频塞入码流八帧一塞入操作,默认2 5帧率
BOOL __stdcall ANTS_SERVER_InputOneAudioFrameV1(DWORD dwStreamType,DWORD dwChannel,BYTE *lpBuffer,DWORD dwSize,BYTE byAudioCodecId,DWORD dwFrameRate);//!新增音频塞入码流八帧一塞入操作
BOOL __stdcall ANTS_SERVER_InputOneAudioFrameV2(DWORD dwStreamType,DWORD dwChannel,BYTE *lpBuffer,DWORD dwSize,BYTE byAudioCodecId,DWORD dwFrameRate);//!新增音频塞入码流一帧一塞入操作
BOOL __stdcall ANTS_SERVER_InputOneAudioFrameV3(DWORD dwStreamType,DWORD dwChannel,BYTE *lpBuffer,DWORD dwSize,BYTE byAudioCodecId);//!新增音频塞入码流一帧一塞入操作默认帧率25
BOOL __stdcall ANTS_SERVER_InputOneAACFrame(DWORD dwStreamType, DWORD dwChannel, BYTE *lpBuffer, DWORD dwSize, DWORD dwFrameRate) ;
BOOL __stdcall ANTS_SERVER_InputOneFrame(DWORD dwStreamType,DWORD dwChannel,BYTE *lpBuffer,DWORD dwSize,BYTE byFrameType);//!内部专用送码流接口
BOOL __stdcall ANTS_SERVER_InputAlarmData(DWORD dwCommand,BYTE *lpBuffer,DWORD dwSize);
BOOL __stdcall ANTS_SERVER_Release( );

#ifdef __cplusplus
}
#endif

#endif

