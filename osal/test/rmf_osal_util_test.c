/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rmf_osal_util.h"
#include "rdk_debug.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_event.h"

#define EXIT_EVENT 0xABCDFFFF

#define RMF_OSAL_ASSERT_ENV(ret,msg) {\
	if (RMF_SUCCESS != ret) {\
		printf("%s:%d : %s \n",__FUNCTION__,__LINE__,msg); \
		abort(); \
	}\
}

static const char *env_keys[] = {"MPE.SYS.ID",
"MPE.SYS.NUM.TUNERS",
"MPE.SYS.ACOUTLET",
"MPE.SYS.RFBYPASS",
"MPE.GFX.BUFFERED",
"VMOPT.0",
"VMOPT.1",
"VMOPT.2",
"VMOPT.3",
"VMOPT.4",
"VMOPT.5",
"VMOPT.6",
"VMOPT.7",
"VMOPT.8",
"VMOPT.9",
"VMOPT.10",
"VMOPT.11",
"VMOPT.12",
"VMOPT.13",
"VMOPT.14",
"VMOPT.15",
"VMOPT.16",
"MainClassArgs.0",
"OCAP.rcInterface.subType",
"OCAP.rcInterface.dataRate",
"OCAP.extensions",
"MPE.ROOTCERTS",
"OCAP.gb.frequency",
"OCAP.persistent.tmpdir",
"OCAP.persistent.metadataFilePrefix",
"OCAP.splash.file",
"VMDLLPATH",
"FS.DEFSYSDIR",
"FS.ROMFS.LIBDIR",
"SYSFONT",
"SYSFONT.tiresias",
"INVOKE.DEBUG.PRE.MPE",
"INVOKE.DEBUG.PRE.JVM",
"INVOKE.DEBUG.PRE.MAIN",
"SITP.PSI.ENABLED",
"SITP.PSI.PROCESS.TABLE.REVISIONS",
"SITP.PSI.PROCESS.OOB.TABLE.REVISIONS",
"SITP.PSI.RETRY.INTERVAL",
"SITP.PSI.ROUND.ROBIN.INTERVAL",
"SITP.PSI.FILTER.NUMBER",
"SITP.PSI.OOB.FILTERS.ALWAYS.SET",
"SITP.PSI.DISABLE.OOB.PSI",
"SITP.SI.ENABLED",
"SITP.SI.STATUS.UPDATE.TIME.INTERVAL",
"SITP.SI.MIN.UPDATE.POLL.INTERVAL",
"SITP.SI.MAX.UPDATE.POLL.INTERVAL",
"SITP.SI.PROCESS.TABLE.REVISIONS",
"SITP.SI.MAX.NTT.WAIT.TIME",
"SITP.SI.MAX.SVCT.DCM.WAIT.TIME",
"SITP.SI.MAX.SVCT.VCM.WAIT.TIME",
"SITP.SI.VERSION.BY.CRC",
"SITP.SI.FILTER.MULTIPLIER",
"SITP.SI.MAX.NIT.SECTION.SEEN.COUNT",
"SITP.SI.MAX.VCM.SECTION.SEEN.COUNT",
"SITP.SI.MAX.DCM.SECTION.SEEN.COUNT",
"SITP.SI.MAX.NTT.SECTION.SEEN.COUNT",
"SITP.SI.REV.SAMPLE.SECTIONS",
"SITP.SI.DUMP.CHANNEL.TABLES",
"SITP.SI.STT.ENABLED",
"SITP.SI.CACHE.ENABLED",
"SITP.SI.CACHE.LOCATION",
"CC.608.ENABLED",
"CC.708.ENABLED",
"CC.FILE.DUMP",
"LOG.MPE.DEFAULT",
"LOG.MPE.TARGET",
"LOG.MPE.CC",
"LOG.MPE.CDL",
"LOG.MPE.SNMP",
"LOG.MPE.DISP",
"LOG.MPE.DVR",
"LOG.MPE.HN",
"LOG.MPE.EVENT",
"LOG.MPE.FILESYS",
"LOG.MPE.GFX",
"LOG.MPE.JAVA",
"LOG.MPE.JNI",
"LOG.MPE.JVM",
"LOG.MPE.MEDIA",
"LOG.MPE.MUTEX",
"LOG.MPE.OS",
"LOG.MPE.POD",
"LOG.MPE.SYS",
"EnableMPELog",
"RemoteRepeatRate",
"OCAP.siDatabase.siWaitTimeout",
"OCAP.siDatabase.sttWaitTimeout",
"OCAP.eas.easExtraLogs",
"OCAP.ait.optim",
"DISP.DEFAULT.GFXCONFIG",
"STORAGE.ROOT",
"STORAGE.RESET.ON.START",
"SYSRES.MONITOR.ENABLE",
"SYSRES.MONITOR.PERIOD",
"MPE_MPOD_CA_QUERY_DISABLED",
"AV.ANALOG.PORTS.SUPPORT",
"MPELOG.THREADID.SUPPORT",
"MPELOG.TIMESTAMP.SUPPORT",
"MPELOG.LOGFILE.SUPPORT",
"MPELOG.LOGFILE.NAME",
"MPELOG.SUPRESS.SUPPORT",
"MPELOG.SUPRESS.INTERVAL",
"MPELOG.SUPRESS.INITIAL",
"MEDIA.CLOSEDCAPTION.SUPPORT",
"MEDIA.DVR.SUPPORT",
"FEATURE.1394.SUPPORT",
"FEATURE.SNMP.SUPPORT",
"FEATURE.CABLECARD.SUPPORT",
"FEATURE.STT.SUPPORT",
"FEATURE.XFS.SUPPORT",
"FEATURE.XFS.BUFFERACCUMULATESIZE",
"FEATURE.DIAGNOSTICS.SUPPORT",
"FEATURE.CDL.SUPPORT",
"FEATURE.FRONTPANEL.SUPPORT",
"FEATURE.TUNER.POLLING.SUPPORT",
"FEATURE.DTCP.SUPPORT",
"FEATURE.BOOTMGR.SUPPORT",
"FEATURE.BOOTMGR.IMAGEINTERVAL",
"FEATURE.HOSTBOOT.MSGBOX",
"FEATURE.AUTHORIZED.DEVICE.CHECK",
"FEATURE.CLEAR_CHANNEL_MAP_ON_POD_REMOVED",
"FEATURE.USE_RI_CA_IMPL",
"FEATURE.CLOSED_CAPTIONS.ENABLE_XDS_FILTER",
"FEATURE.KEEP_SYSTEM_TIME_SYNCED_WITH_CABLE_TIME",
"FEATURE.DVR.DISABLE_MEDIATIME_CORRECTION",
"FEATURE.DVR.DISABLE_CONTINUOUS_TIME_UPDATE",
"FEATURE.DVR.DISABLE_MEDIATIME_INIT",
"FEATURE.DVR.DISABLE_EOF_MEDIATIME_FIX",
"FEATURE.DVR.UPDATE_TIME_INTERVAL_MILLISEC",
"FEATURE.ECM.TO.HOST.REBOOT.SIGNAL",
"DVR.DISABLE_TIME_BASE_ZERO",
"FEATURE.DVR.DISABLE_ISMD_VIDDEC_INTERPOLATE_ALL_PTS",
"FEATURE.DVR.FIXED_FRAME_RATE_BOOL",
"FEATURE.DVR.FIXED_FRAME_RATE_FOR_FFWD",
"FEATURE.DVR.FIXED_FRAME_RATE_FOR_SMOOTH_REVERSE",
"FEATURE.DVR.DISABLE_PTS_RANGE_CHECK",
"FEATURE.DVR.COPYTO.DISK",
"FEATURE.DVR.PID_CHANGE_HANDLING",
"DVR.DISABLE_FRAME_FLIP_CHECK_FOR_EOF",
"DVR.DISABLE_FRAME_FLIP_CHECK_FOR_BOF",
"FEATURE.DVR.INDEXER.MODE",
"FEATURE.DVR.INDEXER.RECOVERY.RESET_SPTS",
"FEATURE.DVR.INSERT_NULL_PKTS_FOR_OVERFLOW",
"FEATURE.DVR.NO_ENCRYPTION_MEMCPY",
"DVR.MAX_HOLD_TIME__TRANSITION_TO_1X",
"DVR.BASE_TIME_OFFSET__TRANSITION_TO_1X",
"DVR.SEQ_HDR_BASED_PLAYBACK_FEED",
"DVR.RECORD.INDEX.VERSION",
"FEATURE.DVR.INDEXER.DEMUX_PORT.WATERMARK",
"FEATURE.ENABLE.CARD.RESET.ON.CCIFAILURE",
"SYSTEM.REBOOT.ON.MSPOD.ERROR",
"ENABLE.CARD.RESET.ON.CARDHOST.COMM.FAILURE",
"DISABLE_CARD_NVRAM_UPDATES",
"SYSTEM.REBOOT.ON.ECM.ERROR.COUNT",
"DELAY_BEFORE_SENDING_MSPOD_CARD_RESET",
"FEATURE.DVR.GET_DURATION_FROM_BATCH",
"FEATURE.DVR.GET_TSB_END_TIME_FROM_BATCH",
"FEATURE.CIPHER.SYNC",
"DTCP.PORT",
"AUDIO.DISABLE.FLAG",
"ISMD_LEAKY_FILTER.ENABLED",
"MAX_TUNE_RETRY_ATTEMPTS",
"TRANSPORT.BUFFERING.WAIT.TIMEOUT",
"TRANSPORT.BUFFERING.OVERRUN.CORRECTION_AMOUNT",
"TRANSPORT.BUFFERING.UNDERRUN.CORRECTION_AMOUNT",
"AVPIPELINE.BASE.TIMEOFFSET.VAL1",
"AVPIPELINE.BASE.TIMEOFFSET.VAL2",
"AV.DERINGING.NOISE.LEVEL",
"AV.DEGRADE.SD.ENABLE",
"AV.DEGRADE.SD.LEVEL",
"AV.DEGRADE.HD.ENABLE",
"AV.DEGRADE.HD.LEVEL",
"AV.CROPPING.HD.ENABLE",
"AV.NUM.LINES.TO.CROP.FOR.HD.TOP",
"AV.NUM.LINES.TO.CROP.FOR.HD.BOTTOM",
"AV.CROPPING.SD.ENABLE",
"AV.NUM.LINES.TO.CROP.FOR.SD.TOP",
"AV.NUM.LINES.TO.CROP.FOR.SD.BOTTOM",
"AV.SD.QSET.ENABLE",
"AV.SD.QSET.VSHIFT_VAL",
"AV.SD.QSET.HSHIFT_VAL",
"AV.HD.QSET.ENABLE",
"AV.HD.QSET.VSHIFT_VAL",
"AV.HD.QSET.HSHIFT_VAL",
"TSB.EXPIRY.CORRECTION_SECS",
"DVR.MAX_TRIES_PTS",
"DVR.MAX_TRIES_PTS_PLAY_LOOP",
"DVR.SLEEP_PTS_RETRIES_MSEC",
"DVR.END_MEDIA_TIME_CORRECTION_SECS",
"DVR.START_MEDIA_TIME_CORRECTION_SECS",
"POWER_BUTTON.BRIGHTNESS",
"TEXT_DISPLAY.BRIGHTNESS",
"SNMP.ENABLE_DVL_ACCESS",
"FEATURE.POPUP_MMI_SUPPORT",
"FEATURE.DUMMYSCREEN.DISABLE.RIFLUSH",
"VIDDEC_REOPEN_ON_CHANCHANGE_FLAG",
"VIDDEC.EMIT.FRAME.POLICY",
"FEATURE.VIDREND.KEEPLASTFRAME.ONSTOP",
"HDMI.INITIALIZATION.AT.STACK.STARTUP",
"HDMI.AUDIO.ENABLE.WAIT.TIME.IN.MSEC",
"SVEN.LOG.MSG",
"FEATURE.AUDIO.CONFIG",
"FEATURE.DVR.PROFILING.RECORD.ENABLE",
"FEATURE.DVR.PROFILING.PLAYBACK.ENABLE",
"FEATURE.DVR.TSB.RESERVEDSPACE",
"FEATURE.DVR.ISMDBUFSIZE",
"DVR.FFWD_INTERFRAME_DELAY_MICROSECS",
"FEATURE.UI.FOCUS",
"XRE.JAVA.GRAPHICS.RESIZE",
"DVR.STORAGE.PARTITION.IGNORE",
"FEATURE.DVR.RECORDING.MOUNTPATH",
"FEATURE.DVR.FORCE.ENCRYPTION",
"FEATURE.DVR.ISMD.BUFFERS",
"FEATURE.DVR.BASETIME.OFFSET",
"FEATURE.DVR.USEVIDSINKFRAMEADVANCE",
"FEATURE.DVR.PLAYBACK.STORE_TS",
"FEATURE.DVR.PLAYBACK.STORE_TS.LOCATION",
"DVR.REC.LOCKTRACE",
"DVR.REC.LOCKTRACE_WITH_TID",
"DVR.TS.TIMEOUT.VALUE",
"DVR.USE.XFS.IOCTL",
"VIDEO.PIPELINE.RECOVERY.FLAG",
"VIDEO.PIPELINE.RECOVERY.BUFMON.OVERRUN.FLAG",
"FEATURE.NATIVE_THREAD_SEGFAULT_HANDLER",
"FEATURE.DVR.RECORD_THREAD_PRIORITY",
"FEATURE.DVR.ENABLE_PLAYBACK_RECOVERY",
"FEATURE.DVR.ENABLE_PLAYBACK_RECOVERY_NEGATIVE_JUMP",
"DVR.PLAYBACK_RECOVERY_SKIP_SECS",
"FEATURE.DISP.RESOLUTION.MONITOR.1080p",
"DVR.ENABLE_ADJUST_OFFSET_512K",
"DVR.ADJUST_OFFSET_512K_MULTIPLES",
"DVR.EVENT_WAIT_FRAME_FLIP_FOR_EOF",
"FEATURE.DISP.VIDSINK.CLOSE.ENABLE",
"FEATURE.DISP.1080p.HDCP.LINK.INTEGRITY.COUNT",
"FEATURE.DISP.1080p.HDCP.LINK.INTEGRITY.TIMEWAIT",
"FEATURE.MRDVR.MEDIASERVER.IFNAME",
"FEATURE.HN.IF.LIST",
"FEATURE.GATEWAY.DOMAIN",
"FEATURE.3D.STEREOSCOPIC.AVRESTART",
"CTP.VL.AtVL",
"CTP.VL.AtVL.Locator",
"CTP.VL.AtVL.SrcId",
"CTP.VL.AtVL.Frequency",
"CTP.VL.AtVL.ProgramNumber",
"CTP.VL.AtVL.Modulation",
"CTP.VL.NoXait",
"CTP.VL.InputPort",
"CTP.VL.UDPTimeout",
"SAVE_POWERSTATE_ON_REBOOT",
"FEATURE.VL_OSAL_MEMMAP.THRESHOLD",
"FEATURE.MRDVR.MEDIASTREAMER.HTTP_CHUNKED_TRANSFER",
/*Dynamic values*/
"ocap.memory.total",
"ocap.memory.video",
"ocap.system.highdef",
"MPE.SYS.OSVERSION",
"MPE.SYS.OSNAME", 
"MPE.SYS.MPEVERSION", 
"MPE.SYS.AC.OUTLET", 
"MPE.SYS.RFMACADDR",
"MPE.SYS.IPADDR",
"MPE.SYS.NUM.TUNERS",
"TEST.INVOKE.ERR",
NULL
};

#define TEST_KEY "util_test"
#define TEST_VALUE "good"

static void util_event_print_thread(void * arg)
{
	uint32_t event_type;
	rmf_Error ret;
	rmf_osal_event_handle_t event_handle;
	rmf_osal_eventqueue_handle_t eventqueue_handle = (rmf_osal_eventqueue_handle_t)arg;
	rmf_osal_event_params_t event_params;

	printf("%s: Started\n", __FUNCTION__);

	while (1)
	{
		ret = rmf_osal_eventqueue_get_next_event(  eventqueue_handle,
				&event_handle, NULL, &event_type,  &event_params);
		RMF_OSAL_ASSERT_ENV(ret, "rmf_osal_eventqueue_get_next_event failed");

		ret = rmf_osal_event_delete( event_handle);
		RMF_OSAL_ASSERT_ENV(ret, "rmf_osal_event_delete failed");

		if ( RMF_OSAL_POWER_STANDBY == event_type)
		{
			printf("\n>>>>>>RMF_OSAL_POWER_STANDBY<<<<<<\n");
		}
		else if ( RMF_OSAL_POWER_FULL == event_type)
		{
			printf("\n>>>>>>RMF_OSAL_POWER_FULL<<<<<<\n");
		}
		else if ( EXIT_EVENT == event_type)
		{
			printf("\n>>>>>>Exiting event thread<<<<<<\n");
			break;
		}
		else
		{
			printf("\n>>>>>>Unknown Event<<<<<<\n");
		}
	}

	printf("%s: End\n", __FUNCTION__);
}

int rmf_osal_util_test()
{
	int i;
	rmf_Error ret;
	const char * env_val;
	static rmf_osal_eventqueue_handle_t eventqueue_handle;

	for (i=0; NULL != env_keys[i]; i++)
	{
		env_val = rmf_osal_envGet(env_keys[i]);
		if (env_val)
			printf("\n%s\t%s",	env_keys[i], env_val );
		else
		{
			printf ("\nrmf_osal_envGet (%s) returned NULL",env_keys[i]);
			break;
		}
	}

	printf("\n Setting env key %s value %s\n", TEST_KEY, TEST_VALUE);
	ret = rmf_osal_envSet ( TEST_KEY, TEST_VALUE);
	RMF_OSAL_ASSERT_ENV (ret, "rmf_osal_envSet Failed");


	env_val = rmf_osal_envGet ( TEST_KEY);
	if ( NULL == env_val )
	{
		RMF_OSAL_ASSERT_ENV ( RMF_OSAL_EINVAL, "rmf_osal_envGet Failed");
	}
	printf("rmf_osal_envGet key %s value %s\n", TEST_KEY, env_val);

	{
		uint32_t bootstatus;
		printf("stbBootstatus: setting 0x1010\n");
		bootstatus = rmf_osal_stbBootStatus(TRUE, 0x1010, 0xFFFF);
		printf("rmf_osal_stbBootStatus returned status 0x%x\n", bootstatus);
		printf("stbBootstatus: not updating 0x0101\n");
		bootstatus = rmf_osal_stbBootStatus(FALSE, 0x0101, 0xFFFF);
		printf("rmf_osal_stbBootStatus returned status 0x%x\n", bootstatus);
		printf("stbBootstatus: setting 0x0101\n");
		bootstatus = rmf_osal_stbBootStatus(TRUE, 0x0101, 0xFFFF);
		printf("rmf_osal_stbBootStatus returned status 0x%x\n", bootstatus);
	}

	{
		rmf_osal_Bool enable;
		ret = rmf_osal_stbGetAcOutletState ( &enable);
		RMF_OSAL_ASSERT_ENV (ret, "rmf_osal_stbGetAcOutletState Failed");
		printf("rmf_osal_stbGetAcOutletState status %d\n", enable);

		enable = TRUE;
		ret = rmf_osal_stbSetAcOutletState ( enable);
		RMF_OSAL_ASSERT_ENV (ret, "rmf_osal_stbSetAcOutletState Failed");
		printf("rmf_osal_stbSetAcOutletState - enabled\n");

		ret = rmf_osal_stbGetAcOutletState ( &enable);
		RMF_OSAL_ASSERT_ENV (ret, "rmf_osal_stbGetAcOutletState Failed");
		printf("rmf_osal_stbGetAcOutletState status %d\n", enable);
	}

	{
		rmf_osal_ThreadId  threadId;
		ret = rmf_osal_eventqueue_create( (const uint8_t*)"RCVQ",
				&eventqueue_handle);
		RMF_OSAL_ASSERT_ENV(ret, "rmf_osal_eventqueue_create failed");

		ret = rmf_osal_registerForPowerKey( eventqueue_handle, NULL);
		RMF_OSAL_ASSERT_ENV(ret, "rmf_osal_registerForPowerKey failed");

		ret = rmf_osal_threadCreate( util_event_print_thread, 
					(void*)eventqueue_handle,
					RMF_OSAL_THREAD_PRIOR_DFLT, 2048, &threadId,
					"Thread_1");
		RMF_OSAL_ASSERT_ENV(ret, "rmf_osal_threadCreate failed");

		sleep (1);
	}

	{
		rmf_osal_PowerStatus newPowerMode = RMF_OSAL_POWER_FULL;
		printf("rmf_osal_stbSetPowerStatus : setting full power mode\n");
		ret =  rmf_osal_stbSetPowerStatus( newPowerMode);
		RMF_OSAL_ASSERT_ENV(ret, "rmf_osal_stbSetPowerStatus failed");

		sleep (1);

		newPowerMode = RMF_OSAL_POWER_STANDBY;
		printf("rmf_osal_stbSetPowerStatus : setting standby power mode\n");
		ret =  rmf_osal_stbSetPowerStatus( newPowerMode);
		RMF_OSAL_ASSERT_ENV(ret, "rmf_osal_stbSetPowerStatus failed");

		sleep (1);

		printf("rmf_osal_stbTogglePowerMode\n" );
		rmf_osal_stbTogglePowerMode( );

		sleep (1);

		printf("rmf_osal_stbTogglePowerMode\n" );
		rmf_osal_stbTogglePowerMode( );

		newPowerMode = rmf_osal_stbGetPowerStatus( );
		printf("rmf_osal_stbGetPowerStatus returned power mode %s\n",
			(newPowerMode==RMF_OSAL_POWER_STANDBY)?"RMF_OSAL_POWER_STANDBY":
			(newPowerMode==RMF_OSAL_POWER_FULL)?"RMF_OSAL_POWER_FULL":"Unknown");
	}

	{
		char option;
		fflush(stdin);
		printf("\n\n*****press q to reboot.. any other keys to continue***\n\n");
		scanf("%c",&option);
		scanf("%c",&option);
		if (option == 'q')
		{
			rmf_osal_STBBootMode mode = RMF_OSAL_BOOTMODE_RESET;
			ret =  rmf_osal_stbBoot( mode);
			RMF_OSAL_ASSERT_ENV(ret, "rmf_osal_stbBoot failed");
		}
	}

	{
		rmf_osal_event_handle_t event_handle;
		ret = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_POWER, EXIT_EVENT, 
				NULL, &event_handle);
		RMF_OSAL_ASSERT_ENV(ret, "rmf_osal_event_create failed");

		ret = rmf_osal_eventqueue_add( eventqueue_handle,
				event_handle);
		RMF_OSAL_ASSERT_ENV(ret, "rmf_osal_eventqueue_add failed");
	}
	printf ( "\n test complete\n");
	return 0;
}

