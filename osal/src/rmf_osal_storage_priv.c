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

#define __RMF_OSAL_STORAGE_VL_INTEL_C__

//#include "vlMpeosStorageImpl.h"
#include "rdk_debug.h"
#include "rmf_osal_storage_priv.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_storage.h"
#include "rmf_osal_storage_priv.h"
#include "rmf_osal_mem.h"
#include "rmf_osal_util.h"
#include <rmf_osal_file.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <mntent.h>
#ifndef DISABLE_XFS
#include <fstab.h>
#endif
#include <sys/statvfs.h>


//File Monitoring via netlink
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <errno.h>
#include<sys/time.h>
#include<signal.h>
#include<stdlib.h>
#include<memory.h>
#include <string.h>

#include <sys/param.h>
#include <sys/mount.h>

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define BUFSZ   16384
#define RMF_OSAL_MEM_DEFAULT RMF_OSAL_MEM_STORAGE
#define INTERNAL_STORAGE_MOUNTPATH "/dev/sda"
#define SOLID_STATE_DRIVE_FS "/dev/sdb1"
#define ROOT_FS "/dev/root"
#define MAX_DEVICE_COUNT 10
#define RMF_OSAL_DVR_VOLUME_VL_DATA_FILENAME "vlmediavols"
typedef struct mntent MountPointInfo;
typedef struct statvfs FSStat;

typedef enum
{
  FS_XFS=0,
  FS_EXTx=1 //For all other partition exceptxfs
}FSTYPE;
static rmf_osal_Bool storageInitAlready = FALSE;
static vlStorageDevicesList *g_vlDeviceStorageList = NULL;
static rmf_osal_Mutex g_StorageDevicelistmutex = 0;
static int g_DeviceCount =0; //max value is current 10. i.e. 10 devices are supported at a time.
static void rmf_osal_priv_updateStorageDevice(vlStorageDevicesList *inStorageDevice,MountPointInfo *mountInfo, FSTYPE mountType);

static rmf_osal_Bool rmf_osal_priv_updateMountPoints(rmf_osal_Bool isAttach, eStorageDeviceListStatus eStatus);
static rmf_osal_Bool rmf_osal_priv_removeMountPoints(char *mountPath);
static StorageMgrData gStorageMgrData;
static rmf_osal_Mutex g_StorageMgrQ =0;

//Volume Monitor
static void rmf_osal_priv_dvrCheckVolumeStatistics();
static void rmf_osal_priv_monitorVolumeSizeThread(void * arg);

//Update Volume info in a file, added for to get correct volume info after reboot
static void rmf_osal_StorageupdateVolumeDataFile(os_StorageDeviceInfo* device);
static void rmf_osal_StorageinitVolumes();
static const char* g_hddPartition = NULL;
//debug function
static void dumpDeviceinfo();


rmf_Error rmf_osal_priv_sendStorageMgrEvent
        (rmf_osal_StorageHandle device, rmf_osal_StorageEvent event, 
        rmf_osal_StorageStatus status)
{
        
        rmf_osal_event_handle_t event_handle;
        rmf_osal_event_params_t event_params;
        rmf_Error ret;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);

        event_params.data = (void*)device;
        event_params.data_extension= status;
        event_params.free_payload_fn = NULL;
        event_params.priority = 0;
        ret = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_STORAGE, 
                event, &event_params, &event_handle);
        if ( RMF_SUCCESS != ret )
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
                        "rmf_osal_event_create() failed\n");
                return RMF_OSAL_STORAGE_ERR_UNKNOWN;
        }

        ret = rmf_osal_eventmanager_dispatch_event( 0 , event_handle);
        if ( RMF_SUCCESS != ret )
        {
                rmf_osal_event_delete( event_handle );
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
                        "rmf_osal_eventmanager_dispatch_event() failed\n");
                return RMF_OSAL_STORAGE_ERR_UNKNOWN;
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return RMF_SUCCESS;
}

static void rmf_osal_priv_monitorMountPointThread(void* arg) 
{
        struct sockaddr_nl nls;
        struct pollfd pfd;
        char buf[512];
        rmf_osal_Bool isDeviceAttach = FALSE;
        memset(&nls,0,sizeof(struct sockaddr_nl));
        nls.nl_family = AF_NETLINK;
        nls.nl_pid = getpid(); //get current process pid
        nls.nl_groups = -1;
        
        pfd.events = POLLIN; //wait for data arrival
        pfd.fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
        if (pfd.fd==-1)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Failed to create netlink Socket ...\n",__FUNCTION__);
                return;
        }       
        // Listen to netlink socket
        if (bind(pfd.fd, (struct sockaddr *)&nls, sizeof(struct sockaddr_nl)) == 0)
        {
                while (-1!=poll(&pfd, 1, -1)) 
                {
                        int i, len = recv(pfd.fd, buf, sizeof(buf), MSG_DONTWAIT);
                        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n [RI]:%s:Mount Status Changed. %d\n",__FUNCTION__,len);
                        i = 0;
                        while (i<len) 
                        {
                          RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n [RI]: Mount Status...[%s]\n",buf+i);
                          if(!strncmp(buf+i,"ACTION=remove",strlen("ACTION=remove")))
                          {
                            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n [RI]: Device is Removed\n");
                            isDeviceAttach = FALSE;
                            while(i<len)
                            {
                              RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n [RI]: Mount Status...[%s]\n",buf+i);
                              //Check which mount point is removed
                              if(strstr(buf+i,"DEVPATH=/block/")!=NULL)
                              {
                                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n [RI]: mount path Removed %s\n",buf+i);
                                char *valRet=NULL;
                                char *temp = strtok_r(buf+i,"/",&valRet);
                                
                                while(temp!=NULL)
                                {
                                        temp = strtok_r(NULL,"/",&valRet);
                                        if(temp)
                                        {
                                                int len = strlen(temp);
                                                if(!strncmp(temp,"DEVPATH=",strlen("DEVPATH=")) || !strncmp(temp,"block",strlen("block")))
                                                continue;
                                                if(len == 4)
                                                {
                                                        char mountFS[10];
                                                        memset(mountFS,'\0',10);
                                                        snprintf(mountFS,10,"/dev/%s",temp);
                                                        rmf_osal_priv_removeMountPoints(mountFS);
                                                        break;
                                                }
                                        }
                                }
                              }
                              i += strlen(buf+i)+1;
                              
                            }
                            continue;
                          }
                          else if(!strcmp(buf+i,"ACTION=add"))
                          {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.STORAGE","\n [RI]: Device is Added\n");
                            isDeviceAttach = TRUE;
                            while(i<len)
                            {
                              RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n [RI]: Mount Status...[%s]\n",buf+i);
                              //Check which mount point is Added
                              if(strstr(buf+i,"DEVPATH=/block/")!=NULL)
                              {
                                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n [RI]: mount path Added %s\n",buf+i);
                                char *valRet=NULL;
                                char *temp = strtok_r(buf+i,"/",&valRet);
                                while(temp!=NULL)
                                {
                                  temp = strtok_r(NULL,"/",&valRet);
                                  if(temp)
                                  {
                                    int len = strlen(temp);
                                    if(!strcmp(temp,"DEVPATH=") || !strcmp(temp,"block"))
                                      continue;
                                    if(len == 4)
                                    {
                                      char mountFS[10];
                                      memset(mountFS,'\0',10);
                                      snprintf(mountFS,10,"/dev/%s",temp);
                                      
                                      usleep(1000*1000);//wait for 10 seconds, to update /etc/mtab file to be updated
                                      rmf_osal_priv_updateMountPoints(isDeviceAttach,STORAGELIST_INVALID);
                                      break;
                                    }
                                  }
                                }
                              }
                              i += strlen(buf+i)+1;
                              
                            }
                            continue;
                          }
                          i += strlen(buf+i)+1;
                        }
                }
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: poll fail Err Code [0x%x]...\n",__FUNCTION__, errno);
        }
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Failed to bind Socket Err Code [0x%x]...\n",__FUNCTION__, errno);
}

static void rmf_osal_priv_dvrCheckVolumeStatistics()
{
  vlStorageDevicesList *next = g_vlDeviceStorageList;
  os_MediaVolumeInfo *scanVolume=NULL;
  int ierrorCode=0;
  FSStat fiData ={0,};
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
  rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
  while(next != NULL)
  {
    if(next->m_StorageDevice != NULL)
    {
      rmf_osal_mutexAcquire(next->m_StorageDevice->mutex);
      scanVolume = next->m_StorageDevice->volumes;
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s:Mount path is %s \n",__FUNCTION__,next->m_StorageDevice->rootPath);
      if((ierrorCode = statvfs(next->m_StorageDevice->rootPath,&fiData)) != 0) //Failure 
      {
        rmf_osal_mutexRelease(next->m_StorageDevice->mutex);
        next = next->m_next;        
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\t Fail to get Mount Information , error code =%d\n",ierrorCode);
        continue;
      }
      while (scanVolume != NULL)
      {
        rmf_osal_mutexAcquire(scanVolume->mutex);        
        //For getting size of volume is bit tricky and time consuming. i.e. iterate through all directory and add up whole file size.But Delaying this implementation till we store recordings at correct location passes by stack.
        uint64_t usedSize =(uint64_t)(((uint64_t)(fiData.f_blocks-fiData.f_bfree)) * (uint64_t) (fiData.f_bsize));
        //scanVolume->usedSize=computeSpaceUsed(scanVolume->rootPath);//un-comment it latter
        scanVolume->usedSize=usedSize;
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\t Used space Size is =%llu\n", scanVolume->usedSize);
        rmf_osal_mutexRelease(scanVolume->mutex);
        scanVolume = scanVolume->next;
      }
      rmf_osal_mutexRelease(next->m_StorageDevice->mutex);
    }
    next = next->m_next;
  }
  dumpDeviceinfo();
  rmf_osal_mutexRelease(g_StorageDevicelistmutex);
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Exiting... \n",__FUNCTION__);
}

/**
 * Thread to monitor the volume statistics
 */
static void rmf_osal_priv_monitorVolumeSizeThread( void * arg)
{
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
  while(1)
  {
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Going for checking file system... \n",__FUNCTION__);
    rmf_osal_priv_dvrCheckVolumeStatistics();
    usleep(60*1000*1000);//1min
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Wake-up After Sleep... \n",__FUNCTION__);
  }        
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
}
rmf_osal_Bool rmf_osal_priv_validateStorageDeviceHdl(rmf_osal_StorageHandle device)
{
        vlStorageDevicesList *next = g_vlDeviceStorageList;
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        while(next != NULL)
        {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n ip device = %x, stored device %x \n",device,next->m_StorageDevice);
                if(device == next->m_StorageDevice)
                {
                        rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting Success ...\n",__FUNCTION__);
                        return RMF_SUCCESS;
                }
                next= next->m_next;
        }
        rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting Failure ...\n",__FUNCTION__);
        return RMF_OSAL_EINVAL;
}


#ifdef XFS_SUPPORT //enable the support
static unsigned char gfeature[] = {"FEATURE.XFS.SUPPORT"};

static int vlFsIsXfsSupportEnabled()
{
    static int bXfsEnabled = 1;
    static int bXfsEnabledCheck = 0;
    const char* env = NULL;
    if(!bXfsEnabledCheck)
    {
        bXfsEnabledCheck = 1;
        if ( (env = rmf_osal_envGet(gfeature)) != NULL)
        {
            RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.STORAGE","\n%s=%s\n", gfeature, env);
            if(strcmp(env,"FALSE") == 0) 
            {
                RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.STORAGE","\n featureString %s is not supported in this Platform\n", gfeature);
                bXfsEnabled =  0;
            }
        }
        else
        {
            bXfsEnabled = 0;
        }
    }
    return bXfsEnabled;
}
#endif 

static void vlFS_get_totalSpace(const char *mountPath, int64_t *totalSpace)
{
#ifdef XFS_SUPPORT
    if(vlFsIsXfsSupportEnabled()) // XFS        
    {
        int64_t iTotalSpace =0;
        int fd=0;
        RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.STORAGE","%s : Entering .....\n", __FUNCTION__);
        if(mountPath && totalSpace)
        {
            RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.STORAGE","get Total space for XFS %s\n", mountPath);
            fd = open(mountPath,O_RDONLY);
            if((-1 == fd) || ((xfsctl(mountPath,fd,XFS_IOC_FSGEOMETRY ,&fsInfo)) <0))
            {
                RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.STORAGE","%s : Fail to get XFS FS information \n",__FUNCTION__);
            }
            else
            {
                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.STORAGE","XFS block size = %u\n", fsInfo.blocksize);
                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.STORAGE","XFS datablocks = %lld\n", (int64_t) fsInfo.datablocks);
                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.STORAGE","XFS rtblocks = %lld\n", (int64_t) fsInfo.rtblocks);
                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.STORAGE","XFS rtextents = %lld\n",(int64_t) fsInfo.rtextents);
                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.STORAGE","XFS rtextsize = %u\n", fsInfo.rtextsize);
                close(fd);
                iTotalSpace = ((int64_t) (fsInfo.blocksize)) * ((int64_t) (fsInfo.rtblocks));
                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.STORAGE","XFS Total Space for RT = %lld\n", iTotalSpace);
                *totalSpace = iTotalSpace;
            }
        }
    }
    else
#endif
    {
        struct statvfs vfsInfo;
    
        if (statvfs(mountPath, &vfsInfo) < 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", "%s : Fail to get FS information\n", __FUNCTION__);
        }
        else
        {
        *totalSpace = (int64_t)vfsInfo.f_blocks * (int64_t)vfsInfo.f_frsize;
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.STORAGE","FS Total Space for RT = %lld\n", *totalSpace);
        }
    }
      
    RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.STORAGE","%s : Exiting .....\n", __FUNCTION__);
}


static void rmf_osal_priv_updateStorageDevice(vlStorageDevicesList *inStorageDevice,MountPointInfo *mountInfo, FSTYPE fsType)
{
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        uint64_t fsTotalSpace = 0;
        uint64_t fsFreeSpace = 0;
        rmf_Error ret = RMF_SUCCESS;
        FSStat fsInfo;
        
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","Disk %s: \n", mountInfo->mnt_dir);
        if(fsType == 0) //xfs type
        {
          vlFS_get_totalSpace(mountInfo->mnt_dir,(int64_t *)&fsTotalSpace);
          fsFreeSpace = fsTotalSpace;//Assumption we're using this partition only for DVR. DVR plugin will actual free size after subtracting dvr recording size from total space.
          RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\t ##### XFS Total Space: %llu\n",fsTotalSpace);
        }
        else
        {
          if(statvfs(mountInfo->mnt_dir,&fsInfo) == 0) //Success
          {
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\t EXT block size: %ld\n", fsInfo.f_bsize);
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\t EXT total no blocks: %i\n", fsInfo.f_blocks);
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\t EXT free blocks: %i\n", fsInfo.f_bfree);  
            fsTotalSpace = (((uint64_t)(fsInfo.f_bsize)) * ((uint64_t) (fsInfo.f_blocks)));
            fsFreeSpace = (((uint64_t)(fsInfo.f_bsize)) * ((uint64_t) (fsInfo.f_bfree)));
          }
          else
          {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\nFail To Acquire Mount Point Statistics\n");
          }          
        }
        
        ret = rmf_osal_memAllocPGen(RMF_OSAL_MEM_STORAGE, sizeof(rmf_osal_Storage),
                (void**)&inStorageDevice->m_StorageDevice);
        if (RMF_SUCCESS != ret)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
                                  "rmf_osal_memAllocPGen() failed\n");
            return;
        }
        inStorageDevice->m_StorageDevice->mnt_type = NULL;
        memset(inStorageDevice->m_StorageDevice,'\0',sizeof(rmf_osal_Storage));
        if(rmf_osal_mutexNew(&(inStorageDevice->m_StorageDevice->mutex)) != RMF_SUCCESS)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\nCould not create device mutex\n");
                rmf_osal_memFreePGen(RMF_OSAL_MEM_STORAGE, inStorageDevice->m_StorageDevice);
                return;
        }
        inStorageDevice->m_StorageDevice->attached = TRUE;
        //Need to check what is the current state of device
        inStorageDevice->m_StorageDevice->status = RMF_OSAL_STORAGE_STATUS_READY;
        //dev/sdb -->internal device
        if((strncmp(mountInfo->mnt_fsname,INTERNAL_STORAGE_MOUNTPATH,strlen(INTERNAL_STORAGE_MOUNTPATH)) == 0))// ||            (strncmp(mountInfo->mnt_fsname,INTERNAL_STORAGE_MOUNTPATH1,strlen(INTERNAL_STORAGE_MOUNTPATH1)) == 0))
        {
                inStorageDevice->m_StorageDevice->type = RMF_OSAL_STORAGE_TYPE_INTERNAL;
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]:%s: Internal Device ---\n",__FUNCTION__);
        }
        else
        {
                inStorageDevice->m_StorageDevice->type = RMF_OSAL_STORAGE_TYPE_DETACHABLE;
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]:%s: Removable Device ---\n",__FUNCTION__);
        }
        
        strncpy(inStorageDevice->m_StorageDevice->rootPath,mountInfo->mnt_dir,strlen(mountInfo->mnt_dir)>RMF_OSAL_STORAGE_MAX_NAME_SIZE?RMF_OSAL_STORAGE_MAX_NAME_SIZE:strlen(mountInfo->mnt_dir));
        
        strncpy(inStorageDevice->m_StorageDevice->name,mountInfo->mnt_fsname,strlen(mountInfo->mnt_fsname)>RMF_OSAL_STORAGE_MAX_NAME_SIZE?RMF_OSAL_STORAGE_MAX_NAME_SIZE:strlen(mountInfo->mnt_fsname));
        
        strncpy(inStorageDevice->m_StorageDevice->displayName,mountInfo->mnt_fsname,strlen(mountInfo->mnt_fsname)>RMF_OSAL_STORAGE_MAX_DISPLAY_NAME_SIZE?RMF_OSAL_STORAGE_MAX_DISPLAY_NAME_SIZE:strlen(mountInfo->mnt_fsname));

        inStorageDevice->m_StorageDevice->mnt_type = strdup(mountInfo->mnt_type);

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","Display name %s: \n", inStorageDevice->m_StorageDevice->displayName);

        inStorageDevice->m_StorageDevice->size = fsTotalSpace;
        inStorageDevice->m_StorageDevice->mediaFsSize = fsTotalSpace;
        inStorageDevice->m_StorageDevice->freeMediaSize =fsFreeSpace;
        inStorageDevice->m_StorageDevice->defaultMediaFsSize = fsFreeSpace;
        
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","Size %llu: \n", inStorageDevice->m_StorageDevice->size);
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","mediaFsSize %llu: \n", inStorageDevice->m_StorageDevice->mediaFsSize);
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","freeMediaSize %llu: \n", inStorageDevice->m_StorageDevice->freeMediaSize);
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","defaultMediaFsSize %llu:\n", inStorageDevice->m_StorageDevice->defaultMediaFsSize);
        //No Need to create default Volume.Used data file to read volume info if any
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
}

//HDD parition to ignore for DVR if it is defined in rmfconfig.ini file
static rmf_osal_Bool rmf_osal_priv_ignorePartition(char *mnttype)
{
  rmf_osal_Bool result = FALSE;
  char *valRet=NULL;
  char *temp = NULL;
  
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n %s: Check partition to ignore -%s\n",__FUNCTION__,mnttype);
  if(g_hddPartition)
  {
        char *hddPartition = strdup(g_hddPartition);
        if(hddPartition == NULL)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n %s: Unable to create string, NO memory \n",__FUNCTION__);
          return result;
        }
        temp = strtok_r(hddPartition,":",&valRet);
        while(temp != NULL)
        {
               RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n %s: partition to ignore in rmfconfig.ini -%s\n",__FUNCTION__,temp);
               if(strcmp(temp,mnttype) == 0)
               {
                      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n %s: ignoring partition as per rmfconfig.ini -%s\n",__FUNCTION__,temp);
                      result = TRUE;
                      break;
               }
               temp=strtok_r(NULL,":",&valRet);
        }
        if(hddPartition)
        {
                free(hddPartition);
                hddPartition = NULL;
        }
  }
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n %s: Exiting ....\n",__FUNCTION__);
  return  result;       
}

/**
 * Check For File System to be supported as a storage device
 */
static rmf_osal_Bool rmf_osal_priv_AddStorageDevice(char *mntType, char *mntSys)
{
        if(mntType == NULL || mntSys == NULL)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","%s: Invalid params\n",__FUNCTION__);
          return RMF_OSAL_EINVAL;
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.STORAGE","%s: mntSys: %s mntType: %s\n",__FUNCTION__,mntSys, mntType);

        if((strncmp("ext3",mntType,strlen("ext3")) == 0) ||
        (strncmp("vfat",mntType,strlen("vfat")) == 0) ||
        (strncmp("ext2",mntType,strlen("ext2")) == 0) || 
        (strncmp("xfs",mntType,strlen("xfs")) == 0)
        )
        {
          //This code is added to exclude Solid State Drive as it has only 4GB storage. Which doesn't allow long duration recording.
          if(strcmp(SOLID_STATE_DRIVE_FS,mntSys) == 0 || strcmp(ROOT_FS,mntSys) == 0 || (rmf_osal_priv_ignorePartition(mntSys) == TRUE))
          {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","%s:This is SSD or rootfs or rmfconfig.ini defined partition Just ignore it - %s\n",__FUNCTION__,mntSys);
            return RMF_OSAL_EINVAL;
          }
          return RMF_SUCCESS;
        }
        else
        return RMF_OSAL_EINVAL;
}

static rmf_osal_Bool rmf_osal_priv_checkIfMountPointInList(char *mountPath)
{
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s: Entering .... \n",__FUNCTION__);
  if(mountPath == NULL || g_vlDeviceStorageList == NULL)
  {
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s: Exiting with Err0.... \n",__FUNCTION__);
    return RMF_OSAL_EINVAL;
  }
  vlStorageDevicesList *next = g_vlDeviceStorageList ,*prev = NULL;
        
  if(storageInitAlready == FALSE)
  {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting Err1 ...\n",__FUNCTION__);
    return RMF_OSAL_EINVAL;
  }
  rmf_osal_mutexAcquire(g_StorageDevicelistmutex);
  while(next != NULL)
  {
    if((next->m_StorageDevice != NULL) && 
        (strncmp(next->m_StorageDevice->name,mountPath,strlen(mountPath)) ==0))
    {
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s:Mount is Present %s \n",__FUNCTION__,next->m_StorageDevice->name);
      rmf_osal_mutexRelease(g_StorageDevicelistmutex);
      return RMF_SUCCESS;
    }
    prev = next;
    next = next->m_next;
  }
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s: Exiting .... \n",__FUNCTION__);
  rmf_osal_mutexRelease(g_StorageDevicelistmutex);
  return RMF_OSAL_EINVAL;
}
static rmf_osal_Bool rmf_osal_priv_removeMountPoints(char *mountPath)
{
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s: Entering .... \n",__FUNCTION__);
        if(mountPath == NULL || g_vlDeviceStorageList == NULL)
        {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s: Exiting with Err0.... \n",__FUNCTION__);
                return RMF_OSAL_EINVAL;
        }
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex);
        vlStorageDevicesList *next = g_vlDeviceStorageList ,*prev = NULL, *temp=NULL;
        
        if(storageInitAlready == FALSE)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting Err1 ...\n",__FUNCTION__);
                rmf_osal_mutexRelease(g_StorageDevicelistmutex);
                return RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
        }
        //Check if head
        if((next != NULL) && (next->m_StorageDevice != NULL) && 
           (strncmp(next->m_StorageDevice->name,mountPath,strlen(mountPath)) ==0))
        {
                if(next->m_next != NULL)
                        g_vlDeviceStorageList = next->m_next;
                rmf_osal_memFreePGen(RMF_OSAL_MEM_STORAGE, next);
                next =NULL;
                rmf_osal_mutexRelease(g_StorageDevicelistmutex);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s: Exiting .... \n",__FUNCTION__);
                return RMF_SUCCESS;
        }
        prev = next;
        if(next != NULL)
                next = next->m_next;
        while(next != NULL)
        {
          if((next->m_StorageDevice != NULL) && 
              (strncmp(next->m_StorageDevice->name,mountPath,strlen(mountPath)) ==0))
          {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s:Remove mnt point %s \n",__FUNCTION__,next->m_StorageDevice->name);
                //free all storage device & volumes
            if(next->m_StorageDevice->volumes)
            {
              //May be need to remove all the volumes currently only one vol support
                rmf_osal_memFreePGen(RMF_OSAL_MEM_STORAGE, next->m_StorageDevice->volumes);
                next->m_StorageDevice->volumes=NULL;
            }
            //node removed is ---notify to listeners
            rmf_osal_StorageEvent strEvent = RMF_OSAL_EVENT_STORAGE_DETACHED;
            rmf_osal_StorageStatus strStatus = RMF_OSAL_STORAGE_STATUS_NOT_PRESENT;
            rmf_osal_priv_sendStorageMgrEvent(next->m_StorageDevice,strEvent,strStatus);
            if(next->m_StorageDevice)
            {
                if (NULL != next->m_StorageDevice->mnt_type)
                {
                    free( next->m_StorageDevice->mnt_type);
                    next->m_StorageDevice->mnt_type = NULL;
                }
                rmf_osal_memFreePGen(RMF_OSAL_MEM_STORAGE, next->m_StorageDevice);
                next->m_StorageDevice = NULL;
            }
            temp = next->m_next;
            prev->m_next = temp;
            
            g_DeviceCount--;
            if(next)
            {
                rmf_osal_memFreePGen(RMF_OSAL_MEM_STORAGE, next);
                next=NULL;
            }
            rmf_osal_mutexRelease(g_StorageDevicelistmutex);
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s: Exiting .... \n",__FUNCTION__);
            return RMF_SUCCESS;
          }
          prev = next;
          next = next->m_next;
        }
        
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s: Exiting .... \n",__FUNCTION__);
        rmf_osal_mutexRelease(g_StorageDevicelistmutex);
        return RMF_SUCCESS;
}
static rmf_osal_Bool rmf_osal_priv_updateMountPoints(rmf_osal_Bool isAttach,  eStorageDeviceListStatus eStatus)
{
        //Get All the mount points
        FILE *pfptrMnt = NULL;
        const char * MOUNTPOINT = "/etc/mtab";
        MountPointInfo * mountPoints = NULL;
        vlStorageDevicesList *next =g_vlDeviceStorageList,*temp=NULL;
        rmf_osal_StorageEvent strEvent = RMF_OSAL_EVENT_STORAGE_ATTACHED;
        rmf_osal_StorageStatus strStatus = RMF_OSAL_STORAGE_STATUS_READY;
        rmf_Error ret = RMF_SUCCESS;

        //get the mount point file descriptor
        pfptrMnt = setmntent(MOUNTPOINT,"r");
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex);  
        if(pfptrMnt)
        {
                while(((mountPoints = getmntent(pfptrMnt)) != NULL) && (g_DeviceCount <= MAX_DEVICE_COUNT))
                {
                        if(mountPoints)
                        {
                          if(rmf_osal_priv_AddStorageDevice(mountPoints->mnt_type,mountPoints->mnt_fsname) != RMF_SUCCESS)
                                continue;//Skip this mount point, not for storage
                                
                                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n mnt point is %s: %s\n",mountPoints->mnt_fsname, mountPoints->mnt_dir);
                                if(isAttach == TRUE) //New Device is added
                                {
                                  if((rmf_osal_priv_checkIfMountPointInList(mountPoints->mnt_fsname) == RMF_SUCCESS) && (eStatus != STORAGELIST_COMPLETE))
                                        {
                                          RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n This Mount point is Already in Device List No Need to Add it again\n");
                                          continue;
                                        }
                                        //Add Device into vl device list First Time
                                        if(g_vlDeviceStorageList == NULL)
                                        {
                                                //This is the Head
        
                                                ret = rmf_osal_memAllocPGen(RMF_OSAL_MEM_STORAGE, sizeof(vlStorageDevicesList),
                                                        (void**)&g_vlDeviceStorageList);
                                                if (RMF_SUCCESS != ret)
                                                {
                                                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
                                                                          "rmf_osal_memAllocPGen() failed\n");
                                                    continue;
                                                }                               
                                                g_DeviceCount = 1;
                                                g_vlDeviceStorageList->m_next = NULL;
                                                next =g_vlDeviceStorageList;
                                        }
                                        else
                                        {
                                                //Other Device Node in a List
        
                                                ret = rmf_osal_memAllocPGen(RMF_OSAL_MEM_STORAGE, sizeof(vlStorageDevicesList),
                                                        (void**)&temp);
                                                if (RMF_SUCCESS != ret)
                                                {
                                                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
                                                                          "rmf_osal_memAllocPGen() failed\n");
                                                    continue;
                                                }
                                                g_DeviceCount++;
                                                temp->m_next = NULL;
                                                next->m_next = temp;
                                                next = temp;
                                        }
                                        FSTYPE fsType = FS_XFS;
                                        if(strncmp("xfs",mountPoints->mnt_type,strlen("xfs")) == 0)
                                        {
                                                fsType =FS_XFS;
                                          
                                        }                                          
                                        else //For all other file system
                                        {
                                                fsType = FS_EXTx;
                                        }
                                                //Fill rmf_osal_Storage Device Structure
                                        rmf_osal_priv_updateStorageDevice(next,mountPoints,fsType);
                                                
                                                if(storageInitAlready == TRUE)
                                                {
                                                  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n called After Storage mgr Init has done\n");
                                                  rmf_osal_priv_sendStorageMgrEvent(next->m_StorageDevice,strEvent,strStatus);
                                        }
                                }
                        }
                }
                endmntent(pfptrMnt); //release the file descriptor
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n setmntent returned NULL\n");
        }
        
        rmf_osal_mutexRelease(g_StorageDevicelistmutex);  
        return RMF_SUCCESS;
}

void rmf_osal_storageInit_IF_vl()
{
        rmf_osal_ThreadId ThreadId =0;
        rmf_osal_ThreadId volSizeThreadId = 0;
	const char *env = NULL;
	rmf_osal_Bool volumeCheckEnableFlag = 1;

/*        lvcreate();
        char cmd[2048];
        sprintf(cmd, "lvcreate -k %s>result1", pathName);
        system(cmd);*/
        //Create a mutex to protect Device Info Struct
        if(storageInitAlready == TRUE)
        {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\nStorage Module is Already Initialize !!!\n");
                return;
        }
        if(rmf_osal_mutexNew(&(g_StorageDevicelistmutex)) != RMF_SUCCESS)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\nCould not create Storage Device List mutex\n");
                return;
        }
        if(rmf_osal_mutexNew(&(g_StorageMgrQ)) != RMF_SUCCESS)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\nCould not create Storage Mgr Event Q mutex\n");
                rmf_osal_mutexDelete(g_StorageDevicelistmutex);
                return;
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        g_hddPartition = rmf_osal_envGet("DVR.STORAGE.PARTITION.IGNORE");
        if(g_hddPartition)
        {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", " %s: PARTITION.IGNORE = %s \n",__FUNCTION__,g_hddPartition);
        }
        else
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", " %s: NO PARTITION TO IGNORE for DVR \n",__FUNCTION__);
        }
        env = rmf_osal_envGet("DVR.STORAGE.PARTITION.VOLUMECHECK.DISABLED");
	
	if(env)
	{
		if(strcmp(env, "TRUE") == 0)
		{
		  volumeCheckEnableFlag =0;
		}
	}
        rmf_osal_priv_updateMountPoints(TRUE,STORAGELIST_COMPLETE);
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex);  
        storageInitAlready = TRUE;
        rmf_osal_mutexRelease(g_StorageDevicelistmutex);
	if(volumeCheckEnableFlag) 
	{
		rmf_osal_StorageinitVolumes();
		//Thread to monitor Attach/Detach of storage device
		if(rmf_osal_threadCreate( rmf_osal_priv_monitorMountPointThread,(void*)NULL, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &ThreadId, "vlMountPointThread" ) != RMF_SUCCESS)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n %s:Could not create MountPath monitor thread...", __FUNCTION__);
			rmf_osal_mutexDelete(g_StorageDevicelistmutex);
			rmf_osal_mutexDelete(g_StorageMgrQ);
			return;
		} 
		//Thread to update the current volume size
		if(rmf_osal_threadCreate( rmf_osal_priv_monitorVolumeSizeThread,(void*)NULL, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &volSizeThreadId, "vlVolumeMonitorThread") != RMF_SUCCESS)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n %s: Could not create Volume monitor thread...", __FUNCTION__);
			return;
		} 
	}
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
}

rmf_Error rmf_osal_storageGetDeviceCount_IF_vl(uint32_t* count)
{
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        if(storageInitAlready == FALSE)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
        }
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
        *count = g_DeviceCount;
        rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n [RI]: %s: Device Count = %d\n",__FUNCTION__,*count);
        
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return RMF_SUCCESS;
}

rmf_Error rmf_osal_storageGetDeviceList_IF_vl(uint32_t* count, rmf_osal_StorageHandle *devices)
{
        rmf_Error result = RMF_SUCCESS;
        vlStorageDevicesList *next = g_vlDeviceStorageList;
        int index =0;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        if(storageInitAlready == FALSE)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
        }
        //Input Parameter validation is already done in mpeos layer
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
        while(next != NULL)
        {
                *devices = next->m_StorageDevice;
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n [RI]: %s: Device Info for app %p\n",__FUNCTION__, devices);
                next = next->m_next;
                index++;
                if(index > *count)
                  break;
                devices++;
        }

        rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}

#if 0
rmf_Error rmf_osal_storageGetInfo_IF_vl(rmf_osal_StorageHandle device, rmf_osal_StorageInfoParam param, void* output)
{
        rmf_Error result = RMF_SUCCESS;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        if(storageInitAlready == FALSE)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting With Err...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
        }
        if(rmf_osal_priv_validateStorageDeviceHdl(device) == RMF_SUCCESS)
        {
                return RMF_SUCCESS;
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}

rmf_Error rmf_osal_storageIsDetachable_IF_vl(rmf_osal_StorageHandle device, rmf_osal_Bool* value)
{
        rmf_Error result = RMF_SUCCESS;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        if(storageInitAlready == FALSE || !value)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting With Err0...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
        }

        if(rmf_osal_priv_validateStorageDeviceHdl(device) != RMF_SUCCESS)
        {
                *value = FALSE;
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting With Err1...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
        }
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
        //How to Know Device is Detachable
        if (device->type & RMF_OSAL_STORAGE_TYPE_DETACHABLE)
          *value = TRUE;
        else
          *value = FALSE;
        rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}

rmf_Error rmf_osal_storageMakeReadyToDetach_IF_vl(rmf_osal_StorageHandle device)
{
        rmf_Error result = RMF_SUCCESS;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        if(storageInitAlready == FALSE)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting With Err 0...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
        }
        if(rmf_osal_priv_validateStorageDeviceHdl(device) != RMF_SUCCESS || !(device->type & RMF_OSAL_STORAGE_TYPE_DETACHABLE))
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting With Err 1...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
        }
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
        if(umount(((rmf_osal_Storage*)device)->rootPath) < 0 )
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n Error in Detaching the device \n");
                result = RMF_OSAL_STORAGE_ERR_UNSUPPORTED;
        }
        else
        {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n Success in Detaching the device \n");
                //Update the device sturcture
        }
        rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}

rmf_Error rmf_osal_storageMakeReadyForUse_IF_vl(rmf_osal_StorageHandle device)
{
        rmf_Error result = RMF_SUCCESS;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        if(storageInitAlready == FALSE)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting With Err 0...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
        }
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
        if(rmf_osal_priv_validateStorageDeviceHdl(device) != RMF_SUCCESS )
        {
                rmf_osal_mutexRelease(g_StorageDevicelistmutex);
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting With Err 1...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
        }
        else if((device->status != RMF_OSAL_STORAGE_STATUS_NOT_PRESENT) && (device->status != RMF_OSAL_STORAGE_STATUS_READY) && (device->status != RMF_OSAL_STORAGE_STATUS_OFFLINE))
        {
                rmf_osal_mutexRelease(g_StorageDevicelistmutex);
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting With Err 2...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_DEVICE_ERR;
        }
        
        if(mount(((rmf_osal_Storage*)device)->rootPath) < 0 )
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n Error in mounting the device \n");
                result = RMF_OSAL_STORAGE_ERR_UNSUPPORTED;
        }
        else
        {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n Success in mounting the device \n");
                //Update the device sturcture
        }
        rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}

rmf_Error rmf_osal_storageIsDvrCapable_IF_vl(rmf_osal_StorageHandle device, rmf_osal_Bool* value)
{
        rmf_Error result = RMF_SUCCESS;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        if(storageInitAlready == FALSE || !value)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting With Err0...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
        }
        else if(rmf_osal_priv_validateStorageDeviceHdl(device) != RMF_SUCCESS )
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting With Err1...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
        }
        *value = TRUE;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}

rmf_Error rmf_osal_storageIsRemovable_IF_vl(rmf_osal_StorageHandle device, rmf_osal_Bool* value)
{
        rmf_Error result = RMF_SUCCESS;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        if(storageInitAlready == FALSE || !value)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
        }
        else if(rmf_osal_priv_validateStorageDeviceHdl(device) != RMF_SUCCESS )
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting With Err...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
        }
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
        if (device->type & RMF_OSAL_STORAGE_TYPE_REMOVABLE)
          *value = TRUE;
        else
          *value = FALSE;
        rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}

rmf_Error rmf_osal_storageEject_IF_vl(rmf_osal_StorageHandle device)
{
        rmf_Error result = RMF_SUCCESS;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        if(storageInitAlready == FALSE)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
          return RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
        }
        if(rmf_osal_priv_validateStorageDeviceHdl(device) == RMF_SUCCESS)
        {
                rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
                //Not sure abt this implementation. may be we need to eject the CD/DVD's
                if(umount(((rmf_osal_Storage*)device)->rootPath) < 0 )
                {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n Error in Detaching the device \n");
                }
                else
                {
                        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\n Success in Detaching the device \n");
                        device->size = 0;
                        device->defaultMediaFsSize = 0;
                        device->mediaFsSize = 0;
                        device->status = RMF_OSAL_STORAGE_STATUS_NOT_PRESENT;
                        //Update the device sturcture
                }
                rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
  return result;
}
rmf_Error rmf_osal_storageInitializeDevice_IF_vl(rmf_osal_StorageHandle device, rmf_osal_Bool userAuthorized, uint64_t* mediaFsSize)
{
        rmf_Error result = RMF_SUCCESS;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        if(storageInitAlready == FALSE)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting Err0...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
        }
        if(rmf_osal_priv_validateStorageDeviceHdl(device) != RMF_SUCCESS)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting Err1...\n",__FUNCTION__);
                return RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
        }
        else if (device->status == RMF_OSAL_STORAGE_STATUS_BUSY)
        {
                result = RMF_OSAL_STORAGE_ERR_BUSY;
        }
        else if (device->status == RMF_OSAL_STORAGE_STATUS_OFFLINE)
        {
                result = RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
        }
        else
        {
                rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
                //Re-initialize the device
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Yet To Implement ...\n",__FUNCTION__);
                rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}
#endif

rmf_Error rmf_osal_storageRegisterQueue_IF_vl (rmf_osal_eventqueue_handle_t eventQueue)
{

        rmf_Error ret;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
        
        ret = rmf_osal_eventmanager_register_handler(
            0,
            eventQueue,
            RMF_OSAL_EVENT_CATEGORY_STORAGE);
        if (RMF_SUCCESS != ret )
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE",
                " rmf_osal_eventmanager_register_handler failed\n");
        }
        rmf_osal_mutexAcquire(g_StorageMgrQ);
        gStorageMgrData.queueRegistered = TRUE;
        rmf_osal_mutexRelease(g_StorageMgrQ);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return RMF_SUCCESS;
}

static void rmf_osal_priv_dvrCheckVolumeStatus()
{
        vlStorageDevicesList *next = g_vlDeviceStorageList;
        uint8_t freeSpacePercent=0;
        uint32_t level = 0;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
        //Check for volume status

        rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Entering ...\n",__FUNCTION__);
        while(next != NULL)
        {
          os_MediaVolumeInfo *volumeNext = next->m_StorageDevice->volumes;
          while(volumeNext)
          {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n <<DVR>>: Volume Info ---\n");
            //Some time partition doesnot have reserved sizes
            if(volumeNext->reservedSize!=0)
            {
                freeSpacePercent = (uint8_t) ((volumeNext->usedSize / volumeNext->reservedSize) * 100);
            }
            for(level = freeSpacePercent + 1; level <= MAX_MEDIA_VOL_LEVEL; level++)
            {
              if (volumeNext->alarmStatus[level-1] == ARMED)
              {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s - Sending free space alarm for level %u\n", __FUNCTION__, level);

                /* The last parameter value is sent with the level value
                so that the same listener can differentiate among various
                levels it has registered*/
#if 0
                if (g_dvrVolQ)
                {
                  rmf_osal_eventQueueSend(g_dvrVolQ, RMF_OSAL_MEDIA_VOL_EVT_FREE_SPACE_ALARM, (void*)&volumeNext, g_dvrACT,level);
                }
#else
                //do we need to send an event here?
#endif
                volumeNext->alarmStatus[level-1] = UNARMED;
              }
            }
            volumeNext = volumeNext->next;
          }

          next= next->m_next;
        }
        rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
}

static void rmf_osal_priv_dvrStorageVolumeAlarmThread()
{
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
       //Monitor for media size status ...in every 5.1 sec
        while(1)
        {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Going for Checking Volume Status... \n",__FUNCTION__);
                rmf_osal_priv_dvrCheckVolumeStatus();
                usleep(50*1000*1000);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Wake-up for status... \n",__FUNCTION__);
        }        
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Exiting... \n",__FUNCTION__);
}

#if 0
rmf_Error rmf_osal_dvrMediaVolumeAddAlarm_IF_vl(rmf_osal_MediaVolume volume, uint8_t level )
{
         rmf_Error result = RMF_OSAL_DVR_ERR_NOERR;
        static rmf_osal_Bool g_dvrAlarmThreadCreated = FALSE;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
        
        if(volume == NULL || level < MIN_MEDIA_VOL_LEVEL || level > MAX_MEDIA_VOL_LEVEL)
          return RMF_OSAL_DVR_ERR_INVALID_PARAM;
        
        //Only One Thread will monitor all the DVR Vol Alarms
        if(g_dvrAlarmThreadCreated == TRUE)
        {
          if(rmf_osal_threadCreate((void*)rmf_osal_priv_dvrStorageVolumeAlarmThread,(void*)NULL, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &g_alarmThreadId, "vlStorageVolumeAlarmThread" ) != RMF_SUCCESS)
                {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n %s:Could not create Alarm thread for DVR...", __FUNCTION__);
                        return RMF_OSAL_DVR_ERR_NOT_ALLOWED;
                } 
        } 
                
        level -= 1;   /* level used for indexing */
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex);
        rmf_osal_mutexAcquire(volume->mutex);
        /* check for alarm of particular level is already set*/
        if (volume->alarmStatus[level] == ARMED ||
            volume->alarmStatus[level] == UNARMED)
        {
          //result = RMF_OSAL_DVR_ERR_NOT_ALLOWED;
        }

        volume->alarmStatus[level] = ARMED;
        rmf_osal_StorageupdateVolumeDataFile(volume->device);
        rmf_osal_mutexRelease(volume->mutex);
        rmf_osal_mutexRelease(g_StorageDevicelistmutex);
        //Alarm Thread is created
        g_dvrAlarmThreadCreated = TRUE;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}

/*
 * Removes the given media volume from our global list and frees its data
 * structures and files on-disk
 */
void deleteMediaVolume(os_MediaVolumeInfo* volume)
{
  removeFiles(volume->rootPath);
  rmf_osal_mutexDelete(volume->mutex);
  rmf_osal_memFreePGen(RMF_OSAL_MEM_STORAGE, volume);
}

rmf_Error rmf_osal_dvrMediaVolumeDelete_IF_vl( rmf_osal_MediaVolume volume )
{
        rmf_Error result = RMF_OSAL_DVR_ERR_NOERR;
        os_MediaVolumeInfo *walker=NULL;
        os_MediaVolumeInfo *prev=NULL;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex);
        rmf_osal_mutexAcquire(volume->device->mutex);

        // Find volume in the list for the device
        prev = walker = volume->device->volumes;
        while (walker != NULL && walker != volume)
        {
        // Increment our previous pointer once the walker has left
        // the front of the list
          if (walker != volume->device->volumes)
            prev = prev->next;

          walker = walker->next;
        }

        if (walker == NULL)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", "<<DVR>> %s - Invalid volume\n", __FUNCTION__);
          rmf_osal_mutexRelease(volume->device->mutex);
          rmf_osal_mutexRelease(g_StorageDevicelistmutex);
          return RMF_OSAL_DVR_ERR_INVALID_PARAM;
        }

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s - Volume = %s (%p)\n",__FUNCTION__, volume->rootPath, (os_MediaVolumeInfo*)volume);

        // Remove it from the device's list of volumes
        if (walker == volume->device->volumes)
          volume->device->volumes = walker->next;  // First in list
        else
          prev->next = walker->next;

    // Update device's free size
        if (volume->reservedSize != 0)
          volume->device->freeMediaSize += volume->reservedSize;
        else
          volume->device->freeMediaSize += volume->usedSize;


        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s - Volume deleted.  Device free size now = %lld\n", __FUNCTION__, volume->device->freeMediaSize);
        rmf_osal_StorageupdateVolumeDataFile(volume->device);
        rmf_osal_mutexRelease(volume->device->mutex);
        deleteMediaVolume(volume);
        rmf_osal_mutexRelease(g_StorageDevicelistmutex);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}

rmf_Error rmf_osal_dvrMediaVolumeGetCount_IF_vl(rmf_osal_StorageHandle device, uint32_t * count )
{
        rmf_osal_Storage *next=NULL;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
        if(storageInitAlready == FALSE )
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting Err0...\n",__FUNCTION__);
          return RMF_OSAL_DVR_ERR_UNSUPPORTED;
        }
        if(!count || (device == NULL) || rmf_osal_priv_validateStorageDeviceHdl(device) != RMF_SUCCESS)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting Err1...\n",__FUNCTION__);
          return RMF_OSAL_DVR_ERR_INVALID_PARAM;
        }
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
        next = (rmf_osal_Storage *)device;
        rmf_osal_mutexAcquire(next->mutex);
        *count =0;
        os_MediaVolumeInfo *volume = next->volumes;
        while(volume != NULL)
        {
          (*count)++;
          volume= volume->next;
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: Total Vol %d...\n",*count);
        rmf_osal_mutexRelease(next->mutex);
        rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return RMF_OSAL_DVR_ERR_NOERR;
}

rmf_Error rmf_osal_dvrMediaVolumeGetInfo_IF_vl(rmf_osal_MediaVolume volume,rmf_osal_MediaVolumeInfoParam param, void * output )
{
        rmf_Error result = RMF_OSAL_DVR_ERR_NOERR;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
        if (!output || !volume)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", "<<DVR>> %s - Invalid parameter", __FUNCTION__);
                return RMF_OSAL_DVR_ERR_INVALID_PARAM;
        }
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex);
        rmf_osal_mutexAcquire(volume->mutex);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s Volume = %s (%p)\n",__FUNCTION__, volume->rootPath, volume);
        
        switch(param)
        {
                case RMF_OSAL_DVR_MEDIA_VOL_SIZE:
                *((uint64_t*)output) = volume->reservedSize;
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s RMF_OSAL_DVR_MEDIA_VOL_FREE_SPACE = %llu\n", __FUNCTION__, volume->reservedSize);
                break;
                case RMF_OSAL_DVR_MEDIA_VOL_FREE_SPACE:
                {
                uint64_t freeSize = 0;
        
                 if (volume->reservedSize != 0)
                 freeSize = volume->reservedSize - volume->usedSize;
                 else
                freeSize = volume->device->freeMediaSize;
        
                *((uint64_t*)output) = freeSize;
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s RMF_OSAL_DVR_MEDIA_VOL_FREE_SPACE = %llu\n",__FUNCTION__, freeSize);
                }
                break;
        
                case RMF_OSAL_DVR_MEDIA_VOL_PATH:
                memset(output, '\0', RMF_OSAL_DVR_MEDIA_VOL_MAX_PATH_SIZE);
                    
                strncpy(output, volume->rootPath,strlen(volume->rootPath) > RMF_OSAL_DVR_MEDIA_VOL_MAX_PATH_SIZE ? (RMF_OSAL_DVR_MEDIA_VOL_MAX_PATH_SIZE - 1) :strlen(volume->rootPath));
                
                
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s RMF_OSAL_DVR_MEDIA_VOL_PATH = %s\n",__FUNCTION__, volume->rootPath);
                break;
//depricated in REL_A1...        
#if 0		
                case RMF_OSAL_DVR_MEDIA_VOL_ALLOWED_LIST_COUNT:
                *((uint64_t*)output) = 0; // <STMGR-COMP> Support allowed lists
                break;
        
                case RMF_OSAL_DVR_MEDIA_VOL_ALLOWED_LIST:
                result = RMF_OSAL_DVR_ERR_UNSUPPORTED; // <STMGR-COMP> Support allowed lists
                break;
#endif        
                default:
                result = RMF_OSAL_DVR_ERR_INVALID_PARAM;
                break;
        }
        
        rmf_osal_mutexRelease(volume->mutex);
        rmf_osal_mutexRelease(g_StorageDevicelistmutex);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}

rmf_Error rmf_osal_dvrMediaVolumeGetList_IF_vl(rmf_osal_StorageHandle device, uint32_t * count, rmf_osal_MediaVolume * volumes )
{
        rmf_Error result = RMF_OSAL_DVR_ERR_NOERR;
        uint32_t actualCount = 0;
        os_MediaVolumeInfo *volume=NULL;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
        
        if((result = rmf_osal_dvrMediaVolumeGetCount_IF_vl(device,count)) == RMF_OSAL_DVR_ERR_NOERR)
        {
          rmf_osal_mutexAcquire(g_StorageDevicelistmutex); 
          rmf_osal_mutexAcquire(device->mutex);
          volume = device->volumes;
          while (actualCount < *count && volume != NULL)
          {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<RI>> %s - Adding volume %s (%p)\n",
                      __FUNCTION__, volume->rootPath, volume);
            volumes[actualCount] = volume;
            actualCount++;
            volume = volume->next;
          }
          rmf_osal_mutexRelease(device->mutex);
          rmf_osal_mutexRelease(g_StorageDevicelistmutex);
        }
        *count = actualCount;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}

rmf_Error rmf_osal_dvrMediaVolumeNew_IF_vl(rmf_osal_StorageHandle device, char * path, rmf_osal_MediaVolume * volume )
{
        os_MediaVolumeInfo *newVolume;
        os_MediaVolumeInfo *scanVolume;
        int i =0;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
        if(storageInitAlready == FALSE)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting Err0...\n",__FUNCTION__);
                return RMF_OSAL_DVR_ERR_INVALID_PARAM;
        }
        if((rmf_osal_priv_validateStorageDeviceHdl(device) != RMF_SUCCESS) || 
            (path == NULL) || (volume == NULL))
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting Err1...\n",__FUNCTION__);
                return RMF_OSAL_DVR_ERR_INVALID_PARAM;
        }
        if (strncmp(path, device->rootPath, strlen(device->rootPath)) != 0)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", "<<DVR>> %s - Path does not match GPFS\n", __FUNCTION__);
          return RMF_OSAL_DVR_ERR_INVALID_PARAM;
        }
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex);
        rmf_osal_mutexAcquire(device->mutex);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n <<DVR>>: %s: Volume Creation Path... \n",path);
        //NO LVM support so creating directory inside the storage device
        //Get all the volume info from input device handle.
        scanVolume = device->volumes;
    // Make sure the path isn't a duplicate
        while (scanVolume != NULL)
        {
          if (strcmp(path, scanVolume->rootPath) == 0)
          {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s - Volume with same path already exists: %s\n",
                      __FUNCTION__, path);
            *volume = (rmf_osal_MediaVolume) scanVolume;
            rmf_osal_mutexRelease(device->mutex);
            rmf_osal_mutexRelease(g_StorageDevicelistmutex);
            return RMF_OSAL_DVR_ERR_NOERR;
          }

          scanVolume = scanVolume->next;
        }
            // Add the new volume to the list
        newVolume = (os_MediaVolumeInfo *)vlMalloc(sizeof(os_MediaVolumeInfo));
        if(NULL == newVolume)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", "<<DVR>> %s - Unable to allocate memory for new volume\n", __FUNCTION__);
          rmf_osal_mutexRelease(device->mutex);
          rmf_osal_mutexRelease(g_StorageDevicelistmutex);
          return RMF_OSAL_DVR_ERR_INVALID_PARAM;
        }
        memset(newVolume,'\0',sizeof(os_MediaVolumeInfo));
        newVolume->next = device->volumes;
        device->volumes = newVolume;

        // Set all of the attributes for the new volume
        strncpy(newVolume->rootPath,path,strlen(path)>(RMF_OSAL_FS_MAX_PATH-1)?(RMF_OSAL_FS_MAX_PATH-1):strlen(path));
        snprintf(newVolume->mediaPath,RMF_OSAL_FS_MAX_PATH,"%s/%s", newVolume->rootPath, OS_DVR_MEDIA_DIR_NAME);
        snprintf(newVolume->dataPath,RMF_OSAL_FS_MAX_PATH, "%s/%s", newVolume->rootPath, OS_DVR_MEDIA_DATA_DIR_NAME);
        newVolume->device = device;
        newVolume->usedSize = 0;
        newVolume->reservedSize = 0;
        for (i = 0; i < MAX_MEDIA_VOL_LEVEL; i++)
          newVolume->alarmStatus[i] = UNUSED;

        rmf_osal_mutexNew(&(newVolume->mutex));

        createFullPath(newVolume->mediaPath);
        createFullPath(newVolume->dataPath);


        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s - \n\tVolume created = %s\n\t mediapath = %s \n\t , datapath =%s \n(%p)\n",
                  __FUNCTION__, newVolume->rootPath, newVolume->mediaPath,newVolume->dataPath,newVolume);

        rmf_osal_mutexRelease(device->mutex);
        rmf_osal_mutexRelease(g_StorageDevicelistmutex);
        *volume = newVolume;
        rmf_osal_StorageupdateVolumeDataFile(device);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return RMF_OSAL_DVR_ERR_NOERR;
}

rmf_Error rmf_osal_dvrMediaVolumeRegisterQueue_IF_vl(rmf_osal_EventQueue queueId , void * act )
{
        rmf_Error result = RMF_OSAL_DVR_ERR_NOERR;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
        g_dvrVolQ = queueId;
        g_dvrACT=act;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}
rmf_Error rmf_osal_dvrMediaVolumeRemoveAlarm_IF_vl(rmf_osal_MediaVolume volume, uint8_t level )
{
        rmf_Error result = RMF_OSAL_DVR_ERR_NOERR;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
        if (level < MIN_MEDIA_VOL_LEVEL || level > MAX_MEDIA_VOL_LEVEL || volume == NULL)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", "<<DVR>> %s - Invalid parameters\n", __FUNCTION__);
          return RMF_OSAL_DVR_ERR_INVALID_PARAM;
        }

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s - Volume = %s (%p), level = %d\n",__FUNCTION__, volume->rootPath, volume, level);

        rmf_osal_mutexAcquire(g_StorageDevicelistmutex);
        level -= 1;/* level used for indexing */
        rmf_osal_mutexAcquire(volume->mutex);

        /*remove free space alarm for a level in a specific MSV*/
        volume->alarmStatus[level] = UNUSED;
        rmf_osal_StorageupdateVolumeDataFile(volume->device);
        rmf_osal_mutexRelease(volume->mutex);
        rmf_osal_mutexRelease(g_StorageDevicelistmutex);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n [RI]: %s: Exiting ...\n",__FUNCTION__);
        return result;
}
rmf_Error rmf_osal_dvrMediaVolumeSetInfo_IF_vl(rmf_osal_MediaVolume volume, rmf_osal_MediaVolumeInfoParam param, void * value )
{
        rmf_Error err = RMF_OSAL_DVR_ERR_NOERR;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n <<DVR>>: %s: Entering... \n",__FUNCTION__);
        if (volume == NULL || value == NULL)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", "<<DVR>> %s - Parameter is invalid\n", __FUNCTION__);
          return RMF_OSAL_DVR_ERR_INVALID_PARAM;
        }

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s Volume = %s (%p)\n",__FUNCTION__, volume->rootPath, volume);
        rmf_osal_mutexAcquire(g_StorageDevicelistmutex);
        rmf_osal_mutexAcquire(volume->device->mutex);
        rmf_osal_mutexAcquire(volume->mutex);

        switch(param)
        {
          case RMF_OSAL_DVR_MEDIA_VOL_SIZE:
          {
            int64_t newSize = *(int64_t*) value;

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s - Requesting new volume (%s) reserve size of %lld\n" ,__FUNCTION__, volume->rootPath, newSize);

            if ((newSize < 0) ||
                 (newSize > 0 &&
                 ((newSize <= (int64_t) volume->usedSize) ||
                 (newSize - volume->reservedSize <= volume->device->freeMediaSize))))

            {
              RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE",
                        "<<DVR>> %s :New value to small for existing content\n",__FUNCTION__);
              err = RMF_OSAL_DVR_ERR_INVALID_PARAM;
            }
            else
            {          
                 volume->reservedSize = newSize;
            }

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s - Setting volume reserve size to %lld Volume = %s, Device free size = %lld\n",__FUNCTION__, newSize, volume->rootPath, volume->device->freeMediaSize);
          }
          break;
//depricated in REL_A1...	  
#if 0
          case RMF_OSAL_DVR_MEDIA_VOL_ALLOWED_LIST:
            err = RMF_OSAL_DVR_ERR_UNSUPPORTED;
            break;
#endif
          default:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s - Unknown param value\n", __FUNCTION__);
            err = RMF_OSAL_DVR_ERR_INVALID_PARAM;
            break;
        }

        rmf_osal_mutexRelease(volume->mutex);
        rmf_osal_mutexRelease(volume->device->mutex);
        rmf_osal_mutexRelease(g_StorageDevicelistmutex);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n <<DVR>>: %s: Exiting ...\n",__FUNCTION__);
        return err;
}
#endif

static void dumpDeviceinfo()
{
  vlStorageDevicesList *next = g_vlDeviceStorageList;

  os_MediaVolumeInfo *scanVolume=NULL;
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
  rmf_osal_mutexAcquire(g_StorageDevicelistmutex);

  while(next != NULL)
  {
    if(next->m_StorageDevice != NULL)
    {
      rmf_osal_mutexAcquire(next->m_StorageDevice->mutex);
      scanVolume = next->m_StorageDevice->volumes;
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n[RI]:%s:Mount path is %s \n",__FUNCTION__,next->m_StorageDevice->rootPath);
      
      while (scanVolume != NULL)
      {
        rmf_osal_mutexAcquire(scanVolume->mutex);        
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.STORAGE","\t At %x Volume root path =%s\n",scanVolume, scanVolume->rootPath);
        rmf_osal_mutexRelease(scanVolume->mutex);
        scanVolume = scanVolume->next;
      }
      rmf_osal_mutexRelease(next->m_StorageDevice->mutex);
    }
    next = next->m_next;
  }
  rmf_osal_mutexRelease(g_StorageDevicelistmutex);
}

/**
 * Update the persistent file that tracks all media volume information for
 * the given storage device
 */
static void rmf_osal_StorageupdateVolumeDataFile(os_StorageDeviceInfo* device)
{
  rmf_osal_File f=NULL;
  char *path=NULL;
  uint32_t numVolumes = 0;
  uint32_t bytes=0;
  os_MediaVolumeInfo* volume=NULL;
  int iLen=0;
  int i=0;
  rmf_Error ret = RMF_SUCCESS;
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n %s: Entering ....\n",__FUNCTION__);
  
  
  if(device != NULL)
  {
        iLen = strlen(device->rootPath) + strlen(RMF_OSAL_DVR_VOLUME_VL_DATA_FILENAME) + 2; //for \ & '\0' character
        ret = rmf_osal_memAllocPGen(RMF_OSAL_MEM_STORAGE, iLen,
                (void**)&path);
        if (RMF_SUCCESS != ret)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
                                  "rmf_osal_memAllocPGen() failed\n");
            return;
        }      
  }
  else
  {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n %s : Invalid Input ",__FUNCTION__);
    return;
  }
    // Create our volume datafile pathname
  snprintf(path, iLen,"%s/%s", device->rootPath, RMF_OSAL_DVR_VOLUME_VL_DATA_FILENAME);

    // Count the volumes on this device
  for (volume = device->volumes; volume != NULL; volume = volume->next)
    numVolumes++;

    // Write the data file for the volume
  rmf_osal_fileOpen(path, RMF_OSAL_FS_OPEN_CAN_CREATE | RMF_OSAL_FS_OPEN_TRUNCATE | RMF_OSAL_FS_OPEN_WRITE, &f);
  bytes = sizeof(numVolumes);
  rmf_osal_fileWrite(f, &bytes, &numVolumes);

  volume = device->volumes;
  for (i = 0; i < numVolumes; i++)
  {
    uint32_t stringLength=0;

    // Write each member of the os_MediaVolumeInfo structure, strings are stored
    // preceded by a 32bit length value

    stringLength = strlen(volume->rootPath);
    bytes = sizeof(stringLength);
    rmf_osal_fileWrite(f, &bytes, &stringLength);
    rmf_osal_fileWrite(f, &stringLength, volume->rootPath);

    stringLength = strlen(volume->mediaPath);
    bytes = sizeof(stringLength);
    rmf_osal_fileWrite(f, &bytes, &stringLength);
    rmf_osal_fileWrite(f, &stringLength, volume->mediaPath);

    stringLength = strlen(volume->dataPath);
    bytes = sizeof(stringLength);
    rmf_osal_fileWrite(f, &bytes, &stringLength);
    rmf_osal_fileWrite(f, &stringLength, volume->dataPath);

    bytes = sizeof(volume->reservedSize);
    rmf_osal_fileWrite(f, &bytes, (void*)&volume->reservedSize);

    bytes = sizeof(volume->alarmStatus);
    rmf_osal_fileWrite(f, &bytes, (void*)&volume->alarmStatus);

    volume = volume->next;
  }

  rmf_osal_memFreePGen(RMF_OSAL_MEM_STORAGE, path);

  rmf_osal_fileClose(f);
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","\n %s: Exiting ....\n",__FUNCTION__);
}
//Code taken from rmf_osal_dvr simulator implementation
static void rmf_osal_StorageinitVolumes()
{
  int i=0;
  int iLen =0;
  uint32_t deviceCount = MAX_STORAGE_DEVICES;
  os_StorageDeviceInfo *devices[MAX_STORAGE_DEVICES];
  char *mediaVolumesDatafile= NULL;
  rmf_osal_File fptrVolDataFile=NULL;

  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n %s : Entering ...\n", __FUNCTION__);
    
  // Iterate through all devices looking for existing media volumes
  rmf_osal_storageGetDeviceList(&deviceCount, (rmf_osal_StorageHandle *)devices);
  for (i = 0; i < deviceCount; i++)
  {
    uint32_t numVolumes=0;
    int64_t offset = 0;
    uint32_t bytes=0;
    int j=0;
    rmf_Error ret = RMF_SUCCESS;
      iLen = strlen(devices[i]->rootPath) + strlen(RMF_OSAL_DVR_VOLUME_VL_DATA_FILENAME) +2;

    ret = rmf_osal_memAllocPGen(RMF_OSAL_MEM_STORAGE, iLen,
          (void**)&mediaVolumesDatafile);
    if (RMF_SUCCESS != ret)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
                            "rmf_osal_memAllocPGen() failed\n");
        return;
    }    
    
    // Open our media volumes datafile for this device
    snprintf(mediaVolumesDatafile,iLen,"%s/%s",devices[i]->rootPath,RMF_OSAL_DVR_VOLUME_VL_DATA_FILENAME);
    if (rmf_osal_fileOpen(mediaVolumesDatafile, RMF_OSAL_FS_OPEN_READ, &fptrVolDataFile) != RMF_SUCCESS)
    {
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.STORAGE", "\n %s - No media volumes for device: %s\n", __FUNCTION__, devices[i]->displayName);
      rmf_osal_memFreePGen(RMF_OSAL_MEM_STORAGE, mediaVolumesDatafile);
      continue;
    }

        // Seek to the start of the file and read the number of volumes
    rmf_osal_fileSeek(fptrVolDataFile, RMF_OSAL_FS_SEEK_SET, &offset);
    bytes = sizeof(numVolumes);
    rmf_osal_fileRead(fptrVolDataFile, &bytes, (void*)&numVolumes);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n %s - Device (%s) found %d volumes\n", __FUNCTION__, devices[i]->name, numVolumes);

        // Load info for each pre-existing media volume
    for (j = 0; j < numVolumes; j++)
    {
      uint32_t stringLength=0;
      os_MediaVolumeInfo *volume=NULL;
      rmf_osal_Dir dirHand;
            // Create volume data structure and populate it from the volume
            // data file
      ret = rmf_osal_memAllocPGen(RMF_OSAL_MEM_STORAGE, sizeof(os_MediaVolumeInfo),
            (void**)&volume);
      if (RMF_SUCCESS != ret)
      {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
                              "rmf_osal_memAllocPGen() failed\n");
          continue;
      }

      bytes = sizeof(stringLength);
            // Paths
      rmf_osal_fileRead(fptrVolDataFile, &bytes, (void*)&stringLength);
      rmf_osal_fileRead(fptrVolDataFile, &stringLength, (void*)volume->rootPath);
      volume->rootPath[stringLength] = '\0';
      rmf_osal_fileRead(fptrVolDataFile, &bytes, (void*)&stringLength);
      rmf_osal_fileRead(fptrVolDataFile, &stringLength, (void*)volume->mediaPath);
      volume->mediaPath[stringLength] = '\0';
      rmf_osal_fileRead(fptrVolDataFile, &bytes, (void*)&stringLength);
      rmf_osal_fileRead(fptrVolDataFile, &stringLength, (void*)volume->dataPath);
      volume->dataPath[stringLength] = '\0';

      //Sanity Check if there is mismatch in volume data file and actual volume present on device
      if (rmf_osal_dirOpen(volume->rootPath, &dirHand) != RMF_SUCCESS)
      {
        //Our Volume data file goof-up need to clean the 
        rmf_osal_memFreePGen(RMF_OSAL_MEM_STORAGE, volume);
        volume = NULL;
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","\n %s : There is a mis-match in data file and actual device ",__FUNCTION__);
        continue;
      }
      else
      {
        //Just close the directory and move forward
        rmf_osal_dirClose(dirHand);
      }
      // Volume size
      bytes = sizeof(volume->reservedSize);
      rmf_osal_fileRead(fptrVolDataFile, &bytes, (void*)&volume->reservedSize);

            // Alarm status
      bytes = sizeof(volume->alarmStatus);
      rmf_osal_fileRead(fptrVolDataFile, &bytes, (void*)&volume->alarmStatus);

            // Initialize the rest of the data
      volume->device = devices[i];
      volume->next = NULL;
      rmf_osal_mutexNew(&(volume->mutex));

            // Compute the volume's space used
      volume->usedSize = computeSpaceUsed(volume->rootPath);

      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<DVR>> %s - Volume (%p) path = %s, reserved size = %lld, used size = %lld\n",__FUNCTION__, volume, volume->rootPath, volume->reservedSize, volume->usedSize);

                // Update the device's free space
      if (volume->reservedSize == 0)
        devices[i]->freeMediaSize -= volume->usedSize;
      else
        devices[i]->freeMediaSize -= volume->reservedSize;

      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.STORAGE", " %s - Device free size is now %lld\n",__FUNCTION__, devices[i]->freeMediaSize);

                // Add this volume to the device's list of volumes
      volume->next = devices[i]->volumes;
      devices[i]->volumes = volume;
    }
    rmf_osal_memFreePGen(RMF_OSAL_MEM_STORAGE, mediaVolumesDatafile);
    mediaVolumesDatafile = NULL;
    iLen=0;
    rmf_osal_fileClose(fptrVolDataFile);
  }
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n %s : Exiting ...\n", __FUNCTION__);
}

//This API if for DVR. Specially rmf_osal_dvrGet() should get generic recording information (for example,available space) for a specified storage device .where input is an optional input pointer used to define storage identifiers. If NULL, the information is retrieved from the default internal storage device.

rmf_osal_StorageHandle  rmf_osal_StorageGetdefaultDevice()
{
  vlStorageDevicesList *next = g_vlDeviceStorageList;
  rmf_osal_StorageHandle handle =NULL;
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
  rmf_osal_mutexAcquire(g_StorageDevicelistmutex);    
  if(next != NULL)
  {
    if(next->m_StorageDevice != NULL)
      handle = next->m_StorageDevice;
  }
  rmf_osal_mutexRelease(g_StorageDevicelistmutex); 
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Exiting... \n",__FUNCTION__);
  return handle;
}

#if 0
void ri_storageInit(void)
{

	RILOG_DEBUG("\n [RI]: %s: Entering ...\n", __FUNCTION__);
	rmf_osal_storageInit_IF_vl();
	RILOG_DEBUG("\n [RI]: %s: Exiting ...\n", __FUNCTION__);
}

//rmf_Error ri_storageRegisterQueue(rmf_osal_EventQueue queueId, void *act)
int ri_storageRegisterQueue(uint32_t queueId, void *act)
{
	rmf_Error ret = RMF_SUCCESS;
	RILOG_DEBUG("\n [RI]: %s: Entering ...\n", __FUNCTION__);
	ret = rmf_osal_storageRegisterQueue_IF_vl( queueId, act);
	RILOG_DEBUG("\n [RI]: %s: Exiting ...\n", __FUNCTION__);
	return (int)ret;
}

//rmf_Error ri_storageGetDeviceCount(uint32_t* count)
int ri_storageGetDeviceCount(uint32_t* count)
{
	rmf_Error ret = RMF_SUCCESS;
	RILOG_DEBUG("\n [RI]: %s: Entering ...\n", __FUNCTION__);
	ret = rmf_osal_storageGetDeviceCount_IF_vl(count);
	RILOG_DEBUG("\n [RI]: %s: Exiting ...\n", __FUNCTION__);
	return (int)ret;
}
//rmf_Error ri_storageGetDeviceList(uint32_t* count, rmf_osal_StorageHandle *devices)
int ri_storageGetDeviceList(uint32_t* count, void* devices)
{
	rmf_Error ret = RMF_SUCCESS;
	RILOG_DEBUG("\n [RI]: %s: Entering ...\n", __FUNCTION__);
	ret = rmf_osal_storageGetDeviceList_IF_vl(count, (rmf_osal_StorageHandle*)devices);
	RILOG_DEBUG("\n [RI]: %s: Exiting ...\n", __FUNCTION__);
	return (int)ret;
}
#endif

//rmf_Error ri_storageMakeReadyToDetach(char* rootPath)
int ri_storageMakeReadyToDetach(char* rootPath)
{
	rmf_Error ret = RMF_SUCCESS;
	RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.STORAGE"," %s: Entering ...\n", __FUNCTION__);

	if (umount(rootPath) < 0)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE","  Error in Detaching the device \n");
		ret = RMF_OSAL_STORAGE_ERR_UNSUPPORTED;
	}
	else
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","  Success in Detaching the device \n");
	}

	RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.STORAGE","  %s: Exiting ...\n", __FUNCTION__);
	return (int)ret;
}

//rmf_Error ri_storageEject(char* rootPath)
int ri_storageEject(char* rootPath)
{
	rmf_Error ret = RMF_SUCCESS;
	RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.STORAGE","  %s: Entering ...\n", __FUNCTION__);

	if (umount(rootPath) < 0)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE"," Error in Detaching the device \n");
		ret = RMF_OSAL_STORAGE_ERR_UNSUPPORTED;
	}
	else
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","  Success in detaching the device \n");
	}
	RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.STORAGE","  %s: Exiting ...\n", __FUNCTION__);
	return (int)ret;
}

#if 0
//rmf_Error ri_sendStorageMgrEvent(rmf_osal_StorageHandle device, rmf_osal_StorageEvent event, rmf_osal_StorageStatus status)
int ri_sendStorageMgrEvent(void* device, int event, int status)
{
	rmf_Error ret = RMF_SUCCESS;
	RILOG_DEBUG( "\n [RI]: %s: Entering ...\n", __FUNCTION__);
	return rmf_osal_priv_sendStorageMgrEvent((rmf_osal_StorageHandle)device, RMF_OSAL_EVENT_STORAGE_CHANGED, RMF_OSAL_STORAGE_STATUS_OFFLINE);
	RILOG_DEBUG("\n [RI]: %s: Exiting ...\n", __FUNCTION__);
	return (int)ret;
}
#endif

