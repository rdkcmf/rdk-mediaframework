/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include "ipcclient_impl.h"
#include "proxymib_GetData.h"
#include "vlHalSnmpTunerInterface.h"
#include "snmpIntrfGetAvOutData.h"
#ifdef USE_XFS_REALTIME
#include <xfs/xfs.h>
#endif
#include <sys/statvfs.h>
#include "secure_wrapper.h"

#define SICACHE_PATH "/tmp/mnt/diska3/persistent/si/"
#define RECORDING_SEGMENT_PATH "/opt/data/OCAP_MSV/0/0/DEFAULT_RECORDING_VOLUME/dvr/"
#define DUMMY "DUMMY"
#define TRUE "TRUE"
#define FALSE "FALSE"
#define SUCCESS 1
#define FAILURE (-1)

/** Function : DvrProtocolVersion

	Description discusses about the version details.
	|--------------|--------------------|
	|spec version  | dvrProtocolVersion	|
	|--------------|--------------------|
	|<= 1.2.0 	   |			0		|
	|>= 1.2.1 	   | 			0-5		|
	|--------------|--------------------|
	dvrProtocolVersion=0 "legacy"
	dvrProtocolVersion=1 "device status reduction only"
	dvrProtocolVersion=2 "simplified states"
	dvrProtocolVersion=3 "removal of low-hanging fruit"
	dvrProtocolVersion=4 "most benefit with only minimal RWS changes"
	dvrProtocolVersion=5 "full optimistic modelling"
	
	@return true
*/
unsigned int Sget_xcaliburClientDvrProtocolVersion(char* DvrProtocolVersion, int nChars)
{
	// Hardcoded to 0 (ZERO).
	strncpy(DvrProtocolVersion, "0", sizeof("0"));
	nChars = strlen("0")+1;
	return SUCCESS;
}

/** Function : HNReady

	Actually if HN is disabled, hnReady will always return true. 
	False implies it will eventually become ready, i.e. "please wait". 
	True means the system has reached its final state. 
	Another way of thinking of it is, hnReady tells you if it's ok to start using hnIp and hnPort.
	If hnReady=true && hnIp is blank, that would indicate that HN could not be started due to a self-discovery issue, 
	or you're on an RNG150 that has "none" in config.properties instead of "cvp1". 
	Otherwise I guess you could consider HN to be "enabled", although recordings may or may not be published 
	depending whether dlna is enabled from XRE, dlnadvr is provisioned, mdvr is provisioned, and dvr is provisioned. 
	The last 3 of those 4 criteria can also be checked via SNMP, but not the DLNA toggle currently.

	@return true
*/
unsigned int Sget_xcaliburClientHNReady(char* HNReady, int nChars)
{
	//  hardcoded
	strncpy(HNReady, TRUE, sizeof(TRUE));
	nChars = strlen(TRUE)+1;
	return SUCCESS;
}

/** Function : HNPort


	@return true
*/
unsigned int Sget_xcaliburClientHNPort(char* HNPort, int nChars)
{
	// hardcoded
	strncpy(HNPort, "4004", sizeof("4004"));
	nChars = strlen("4004")+1;
	return SUCCESS;
}

/** Function : HNIP


	@return true
*/
unsigned int Sget_xcaliburClientHNIp(char* HNIp, int nChars)
{
	//  hardcoded
	strncpy(HNIp, "0.0.0.0", sizeof("255.255.255.255"));
	nChars = strlen("0.0.0.0")+1;
	return SUCCESS;
}

/** Function : EIDDisconnect

	@return true
*/
unsigned int Sget_xcaliburClientEIDDisconnect(char* EIDDisconnect, int nChars)
{
	// hardcoded 
	strncpy(EIDDisconnect, FALSE, sizeof(FALSE));
	nChars = strlen(FALSE)+1;
	return SUCCESS;
}

/** Function : DLNALinearAuth

	@return true
*/
unsigned int Sget_xcaliburClientDLNALinearAuth(char* DLNALinearAuth, int nChars)
{
	//  Dummy Code 
	strncpy(DLNALinearAuth, TRUE, sizeof(TRUE));
	nChars = strlen(TRUE)+1;
	return SUCCESS;
}

/** Function : DLNADVRAuth

	@return true
*/
unsigned int Sget_xcaliburClientDLNADVRAuth(char* DLNADVRAuth, int nChars)
{
	// Dummy Code
	strncpy(DLNADVRAuth, TRUE, sizeof(TRUE));
	nChars = strlen(TRUE)+1;
	return SUCCESS;
}

/** Function : DVRRec

	@return true
*/
unsigned int Sget_xcaliburClientDVRRec(char* DVRRec, int nChars)
{
	// Dummy Code
	strncpy(DVRRec, FALSE, sizeof(FALSE));
	nChars = strlen(FALSE)+1;
	return SUCCESS;
}

/** Function : SpecVersion

	@return true
*/
unsigned int Sget_xcaliburClientSpecVersion(char* SpecVersion, int nChars)
{
	strncpy(SpecVersion, "2.0.0", sizeof("2.0.0")); // currently hardcoded to "2.0.0"  
	nChars = strlen("2.0.0")+1;
	return SUCCESS;
}

/** Function : NetworkVersion

	@return true
*/
unsigned int Sget_xcaliburClientNetworkVersion(char* NetworkVersion, int nChars)
{
	// Dummy Code
	strncpy(NetworkVersion, "1.3.3.1203", sizeof("1.3.3.1203"));
	nChars = strlen("1.3.3.1203")+1;
	return SUCCESS;
}

/** Function : NetworkType

	@return true
*/
unsigned int Sget_xcaliburClientNetworkType(char* NetworkType, int nChars)
{
	// Dummy Code
	strncpy(NetworkType, "barcelona", sizeof("barcelona"));
	nChars = strlen("barcelona")+1;
	return SUCCESS;
}

/** Function : DVRNotBlocked

	@return true
*/
unsigned int Sget_xcaliburClientDVRNotBlocked(char* DVRNotBlocked, int nChars)
{
	//  Dummy Code
	strncpy(DVRNotBlocked, TRUE, sizeof(TRUE));
	nChars = strlen(TRUE)+1;
	return SUCCESS;
}

/** Function : DVRProvisioned

	@return true
*/
unsigned int Sget_xcaliburClientDVRProvisioned(char* DVRProvisioned, int nChars)
{
	// Dummy Code 
	strncpy(DVRProvisioned, FALSE, sizeof(FALSE));
	nChars = strlen(FALSE)+1;
	return SUCCESS;
}

/** Function : DVRReady

	@return true
*/
unsigned int Sget_xcaliburClientDVRReady(char* DVRReady, int nChars)
{
	// Dummy Code
	strncpy(DVRReady, TRUE, sizeof(TRUE));
	nChars = strlen(TRUE)+1;
	return SUCCESS;
}

/** Function : mDVRAuth

	@return true
*/
unsigned int Sget_xcaliburClientmDVRAuth(char* DVRAuth, int nChars)
{
	// Dummy Code
	strncpy(DVRAuth, FALSE, sizeof(FALSE));
	nChars = strlen(FALSE)+1;
	return SUCCESS;
}

/** Function : HDAuth

	@return true
*/
unsigned int Sget_xcaliburClientHDAuth(char* HDAuth, int nChars)
{
	// Dummy Code
	strncpy(HDAuth, FALSE, sizeof(FALSE));
	nChars = strlen(FALSE)+1;
	return SUCCESS;
}

/** Function : HDCapable

	Always returns TRUE as all RDK platforms support at least 720p ???
	@return true
*/
unsigned int Sget_xcaliburClientHDCapable(char* HDCapable, int nChars)
{
	strncpy(HDCapable, TRUE, sizeof(TRUE));
	nChars = strlen(TRUE)+1;
	return SUCCESS;
}

/** Function : getFreeSpace

	@return freeSpace
*/
long long getFreeSpace( const char* mountPath )
{
   long long freeSpace= 0;
   int rc;

#ifdef USE_XFS_REALTIME
   xfs_fsop_counts_t xfsFree;
   struct xfs_fsop_geom fsInfo;
   int fd;

   fd= ::open( mountPath, O_RDONLY );
   if ( fd >= 0 )
   {
      rc= xfsctl( mountPath, fd, XFS_IOC_FSCOUNTS, &xfsFree );
      if ( rc >= 0 )
      {
         rc= xfsctl( mountPath, fd, XFS_IOC_FSGEOMETRY, &fsInfo );
         if ( rc >= 0 )
         {
            freeSpace= (((long long)(xfsFree.freertx))*((long long)(fsInfo.rtextsize))*((long long)(fsInfo.blocksize)));
         }
      }
      ::close(fd);
   }
#else
   struct statvfs vfsInfo;

   rc= statvfs(mountPath, &vfsInfo);
   if ( rc >= 0 )
   {
      freeSpace = (long long)vfsInfo.f_bfree * (long long)vfsInfo.f_bsize;
   }
#endif

   return freeSpace;
}

/** Function : FreeDiskSpace

	@return true
*/
unsigned int Sget_xcaliburClientFreeDiskSpace(char* FreeDiskSpace, int nChars)
{
	long long freeSpace = 0;
	freeSpace = getFreeSpace(RECORDING_SEGMENT_PATH);
	sprintf(FreeDiskSpace, "%lld", freeSpace);
	//strncpy(FreeDiskSpace, "931958095872", sizeof("931958095872"));
	//nChars = strlen("931958095872")+1;
	return SUCCESS;
}

/** Function : NumberOfDVRTuners

	Returning One less than total number of avialable Tuners 
	@return true
*/
unsigned int Sget_xcaliburClientNumberOfDVRTuners(char* NumberOfDVRTuners, int nChars)
{
	unsigned int numOfTuners = 0;
	unsigned int retStatus;
	retStatus = SnmpGetTunerCount(&numOfTuners);
	sprintf(NumberOfDVRTuners, "%d", numOfTuners-1);
	nChars = strlen(NumberOfDVRTuners)+1;
	return SUCCESS;
}

/** Function : SILoadComplete

	@return true
*/
unsigned int Sget_xcaliburClientSILoadCompleted(char* SILoadCompleted, int nChars)
{
	// Hardcoded
	strncpy(SILoadCompleted, TRUE, sizeof(TRUE));
	nChars = strlen(TRUE)+1;
	return SUCCESS;
}

/** Function : SICacheSize

	SI Data size siCacheSize 1.3.6.1.4.1.17270.9225.1.4.1.23 RO (String) 
		Integer - Number of services in SIManager cache that are of 
		ServiceType ANALOG_TV, DIGITAL_TV, or UNKNOWN
	@return true
*/
unsigned int Sget_xcaliburClientSICacheSize(char* SICacheSize, int nChars)
{
	long long siCacheSize = 0; // default size i.e. number of services is ZERO
	FILE *fp = NULL;
	char noServices[128]={'\0'}, command[256]={'\0'};
#if 0
	/*
		Enable this code, in case to read SI cache location from rmfconfig.ini file and then take dirname
	*/
	char path[128]= {'\0'};
	const char *siOOBCacheLocation = rmf_osal_envGet("SITP.SI.CACHE.LOCATION");
    if ( (siOOBCacheLocation == NULL))
    {
    	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP","OOB Caching location is not specified\n");
    	siOOBCacheLocation = "/tmp/mnt/diska3/persistent/si/si_table_cache.bin";
    }

	memset(command, '\0', sizeof(command));
	sprintf(command, "dirname %s", siOOBCacheLocation);
   	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP","SICacheSize : command to get dirname is '%s'\n", command);
	fp = popen(command, "r");
	if(fp == NULL)
	{
		siCacheSize = 0;
	}
	else
	{
		//dirname
		 while (fgets(path, sizeof(path)-1, fp) != NULL)
		 {
   			 RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP","SICacheSize : si cache path is '%s'\n", path);
  		 }
	}
	if(NULL != fp)
	{
		pclose(fp);
		fp = NULL;
	}
	///si_cache_parser_121 /tmp/mnt/diska3/persistent/si/
	memset(command, '\0', sizeof(command));
#endif //if 0
	snprintf(command,sizeof(command), "/si_cache_parser_121 %s | grep -c RChannelVCN", SICACHE_PATH /*path*/);
   	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","SICacheSize : command to read number of services is '%s'\n", command);

#ifdef YOCTO_BUILD
        fp = v_secure_popen("r","/si_cache_parser_121 %s | grep -c RChannelVCN", SICACHE_PATH /*path*/);
#else
	fp = popen(command, "r");
#endif
	if(fp == NULL)
	{
		siCacheSize = 0;
	}
	else
	{
		 char *endptr;
		 fgets(noServices, sizeof(noServices)-1, fp); 
   		 RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","SICacheSize : si cache number of services is '%s'\n", noServices);
  		 
		 siCacheSize = strtoll(noServices, &endptr, 10);
	}

	if(NULL != fp)
	{
#ifdef YOCTO_BUILD
                v_secure_pclose(fp);
#else
		pclose(fp);
#endif
		fp = NULL;
	}
   	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","SICacheSize : '%d'\n", siCacheSize);

	sprintf(SICacheSize, "%lld", siCacheSize);
	nChars = strlen(SICacheSize)+1;
	return SUCCESS;
}

/** Function : SysDSTOffset

	@return true
*/
unsigned int Sget_xcaliburClientSysDSTOffset(char* SysDSTOffset, int nChars)
{
	int timezoneOffset = 0;
	IPC_CLIENT_SNMPGetGenFetTimeZoneOffset(&timezoneOffset);
	sprintf(SysDSTOffset, "%d", timezoneOffset);
	nChars = sizeof(SysDSTOffset);
	return SUCCESS;
}

/** Function : SysTime

	@return true
*/
unsigned int Sget_xcaliburClientSysTime(char** ppSysTime, int nChars)
{
	time_t rawtime;

	time (&rawtime);
	*ppSysTime = ctime (&rawtime);
	nChars = strlen(*ppSysTime);
	return SUCCESS;
}

/** Function : EthernetInUse

	@return true
*/
unsigned int Sget_xcaliburClientEthernetInUse(char* EthernetInUse, int nChars)
{
	// from confluence page: always returns "Unknown"
	strncpy(EthernetInUse, "Unknown", sizeof("Unknown"));
	nChars = strlen("Unknown")+1;
	return SUCCESS;
}

/** Function : EthernetEnabled

	@return true
*/
unsigned int Sget_xcaliburClientEthernetEnabled(char* EthernetEnabled, int nChars)
{
	// Dummy Code
	strncpy(EthernetEnabled, TRUE, sizeof(TRUE));
	nChars = strlen(TRUE)+1;
	return SUCCESS;
}

/** Function : MoCAInUse

	@return true
*/
unsigned int Sget_xcaliburClientMoCAInUse(char* MoCAInUse, int nChars)
{
	//  Dummy Code
	strncpy(MoCAInUse, "Unknown", sizeof("Unknown"));
	nChars = strlen("Unknown")+1;
	return SUCCESS;
}

/** Function : MoCAEnabled

	@return true
*/
unsigned int Sget_xcaliburClientMoCAEnabled(char* MoCAEnabled, int nChars)
{
	// Dummy Code
	strncpy(MoCAEnabled, TRUE, sizeof(TRUE));
	nChars = strlen(TRUE)+1;
	return SUCCESS;
}

/** Function : CMACVersion

	@return true
*/
unsigned int Sget_xcaliburClientCMACVersion(char* CMACVersion, int nChars)
{
	//  Hard Code
	strncpy(CMACVersion, "Unknown", sizeof("Unknown"));
	nChars = strlen("Unknown")+1;
	return SUCCESS;
}

/** Function : AvgChannelTuneTime

	@return true
*/
unsigned int Sget_xcaliburClientAvgChannelTuneTime(char* AvgChannelTuneTime, int nChars)
{
	//  Hard Code
	strncpy(AvgChannelTuneTime, "0", sizeof("0"));
	nChars = strlen("0")+1;
	return SUCCESS;
}

/** Function : EASStatus

	0=EAS_MESSAGE_RECEIVED 
	1=EAS_MESSAGE_IN_PROGRESS 
	2=EAS_NOT_IN_PROGRESS
	@return true
*/
unsigned int Sget_xcaliburClientEASStatus(char* EASStatus, int nChars)
{
	//  Dummy Code
	strncpy(EASStatus, "2", sizeof("2"));
	nChars = strlen("2")+1;
	return SUCCESS;
}

/** Function : CCStatus

	@return true
*/
unsigned int Sget_xcaliburClientCCStatus(char* CCStatus, int nChars)
{
	//  Dummy Code
	strncpy(CCStatus, "0", sizeof("0"));
	nChars = strlen("0")+1;
	return SUCCESS;
}

/** Function : CurrentChannel

	Returns the virtual channel number
	@return true
*/
unsigned int Sget_xcaliburClientCurrentChannel(char* CurrentChannel, int nChars)
{
	// Dummy Code
	unsigned short ChannelMapId;
	IPC_CLIENT_OobGetChannelMapId(&ChannelMapId);
	sprintf(CurrentChannel, "%d", ChannelMapId);
	nChars = sizeof(CurrentChannel);
	return SUCCESS;
}

/** Function : VodEnabled

	@return true
*/
unsigned int Sget_xcaliburClientVodEabled(char* VodEabled, int nChars)
{
	// Dummy Code
	strncpy(VodEabled, TRUE, sizeof(TRUE));
	nChars = strlen(TRUE)+1;
	return SUCCESS;
}

/** Function : DVREnabled

	@return true
*/
unsigned int Sget_xcaliburClientDVREnabled(char* DVREnabled, int nChars)
{
	// Dummy Code
	strncpy(DVREnabled, FALSE, sizeof(FALSE));
	nChars = strlen(FALSE)+1;
	return SUCCESS;
}

/** Function : DVRCapable

	@return true
*/
unsigned int Sget_xcaliburClientDVRCapable(char* DVRCapable, int nChars)
{
	// Dummy Code
	strncpy(DVRCapable, TRUE, sizeof(TRUE));
	nChars = strlen(TRUE)+1;
	return SUCCESS;
}

/** Function : NumberOfTuners

    Returning total number of avialable Tuners 
    @return true
*/
unsigned int Sget_xcaliburClientNumberOfTuners(char* NumberOfTuners, int nChars)
{
	unsigned int numOfTuners = 0;
	unsigned int retStatus;
	retStatus = SnmpGetTunerCount(&numOfTuners);
	sprintf(NumberOfTuners, "%d", numOfTuners);
	nChars = strlen(NumberOfTuners)+1;
	return SUCCESS;
}

/** Function : populateDiagnsoticsJson

	Reads the diagnostics.json file and returns the values
	@return 
*/
void populateDiagnsoticsJson(uint16_t *channelMapId, uint16_t *controllerId, uint32_t *vodServerId, uint64_t *plantId)
{
#ifdef USE_PXYSRVC
	int i = 0;
	char json_diag_str[16][32];
	char diag_str[512];
	FILE *fp;
	const char * json_diag_location = NULL;
    json_diag_location = rmf_osal_envGet("RMFPXYSRV.JSONFILE.LOCATION");
	if ( (json_diag_location == NULL))
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s():%d: Diagnostics.json location is not specified. Hence using the default location...\n", __FUNCTION__, __LINE__);
		json_diag_location = "/tmp/mnt/diska3/persistent/usr/1112/703e/diagnostics.json";
	}
	
	fp = fopen(json_diag_location, "r");
    if(fp != NULL)
    {
        while(!feof(fp))
        {
            if ( fscanf(fp,"%s",json_diag_str[i]) <= 0 )
            {
              RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","fscanf returned zero or negative value\n");
            }
            if((*json_diag_str[i] == '{') || (*json_diag_str[i] == '}'))
                    continue;
            //printf("File Buffer : %s\n", json_diag_str[i]);
            i++;
        }
        snprintf(diag_str, sizeof(diag_str),"%s %s %s %s %s %s %s %s %s", json_diag_str[0], json_diag_str[1], json_diag_str[2], json_diag_str[3], json_diag_str[4], json_diag_str[5], json_diag_str[6], json_diag_str[7], json_diag_str[8]);
        sscanf(diag_str, "\"systemIds\": \"channelMapId\": %hu, \"controllerId\": %hu, \"plantId\": %llu, \"vodServerId\": %d\n", channelMapId, controllerId, plantId, vodServerId);
        //printf("File Buffer : %s\n", diag_str);
        //printf("Read file buffer:channelMapId is %hu, controller id=%hu, plantId=%llu, vodServerId=%d\n", channelMapId, controllerId, plantId, vodServerId);
        fclose(fp);
    }
#endif
}

/** Function : PlantId

    @return true
*/
unsigned int Sget_xcaliburClientPlantId(char* PlantId, int nChars)
{
	uint64_t plantId=0;
#ifdef USE_PXYSRVC
	uint16_t channelMapId=0, controllerId=0;
	uint32_t vodServerId=0;
	populateDiagnsoticsJson(&channelMapId, &controllerId, &vodServerId, &plantId);
#endif

	sprintf(PlantId, "%llu", plantId);
	nChars = sizeof(PlantId);
	return SUCCESS;
}

/** Function : VodServerId

    @return true
*/
unsigned int Sget_xcaliburClientVodServerId(char* VodServerId, int nChars)
{
	uint32_t vodServerId=0;
#ifdef USE_PXYSRVC
	uint16_t channelMapId=0, controllerId=0;
	uint64_t plantId=0;
	populateDiagnsoticsJson(&channelMapId, &controllerId, &vodServerId, &plantId);
#endif
	sprintf(VodServerId, "%d", vodServerId);
	nChars = sizeof(VodServerId);
	return SUCCESS;
}

/** Function : ControllerId

    @return true
*/
unsigned int Sget_xcaliburClientControllerID(char* ControllerID, int nChars)
{
	uint16_t controllerId=0;
#ifdef USE_PXYSRVC
	uint16_t channelMapId=0;
	uint32_t vodServerId=0;
	uint64_t plantId=0;
	populateDiagnsoticsJson(&channelMapId, &controllerId, &vodServerId, &plantId);
#endif
	sprintf(ControllerID, "%d", controllerId);
	nChars = sizeof(ControllerID);
	return SUCCESS;
}

/** Function : ChannelMapId

    @return true
*/
unsigned int Sget_xcaliburClientChannelMapId(char* ChannelMapId, int nChars)
{
	uint16_t channelMapId=0;
#ifdef USE_PXYSRVC
	uint16_t controllerId=0;
	uint32_t vodServerId=0;
	uint64_t plantId=0;
	populateDiagnsoticsJson(&channelMapId, &controllerId, &vodServerId, &plantId);
#endif
	sprintf(ChannelMapId, "%d", channelMapId);
	nChars = sizeof(ChannelMapId);
	return SUCCESS;
}

/** Function : T2pVersion

    @return true
*/
unsigned int Sget_xcaliburClientT2pVersion(char* T2pVersion, int nChars)
{
	// Dummy Code
	strncpy(T2pVersion, "Unknown", sizeof("Unknown"));
	nChars = strlen("Unknown")+1;
	return SUCCESS;
}

