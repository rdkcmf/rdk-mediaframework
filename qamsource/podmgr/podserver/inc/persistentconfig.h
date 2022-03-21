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



/** \file pfcpersistentconfig.h General persistent config system. */

#ifndef PFCPERSISTENTCONFIG_H
#define PFCPERSISTENTCONFIG_H

#include "cmhash.h"

/**
 * \brief General persistent config system
 *
 * Specializes CMHash for persistent configuration.
 *
 */

class CMpersistentConfig:public CMHash
{
  public:
    typedef enum _RESULT
    {
        SUCCESS,
        READ_ONLY,
        NOT_INITIALIZED,
        ERROR_INVALID_DIRECTORY_PATH,
        ERROR_FILE_IS_A_DIRECTORY,
    }RESULT;

    typedef enum _MAXES
    {
        MAX_PATH1    = 256
            
    }MAXES;
    
    typedef enum _CONFIG_TYPE
    {
        STATIC,         // rarely changes, for e.g. ocap version
        DYNAMIC,        // occasionally changes, for e.g. zipcode, language preference or preferred turn on channel
        VOLATILE,       // changes all the time, for e.g. last used channel

        INVALID = 0x7FFFFFFF
            
    }CONFIG_TYPE;
    
    /**
     * Constructor
     * \param rootpath the root path of the file in which the hash will be saved
     * \param relativepathname the file in which the hash will be saved
     */
    CMpersistentConfig(CONFIG_TYPE eConfigType = DYNAMIC, char *rootpath = NULL, char * relativepathname = NULL);
    
    /**
     * Init member vars
     * \param rootpath the root path of the file in which the hash will be saved
     * \param relativepathname the file in which the hash will be saved
     * \return SUCCESS on success.
     *
     */
    
    RESULT initialize(CONFIG_TYPE eConfigType, char *rootpath, char * relativepathname);
    
	/**
	 * Save the current hash to disk escaping special characters.
	 * \return zero is returned if the file is saved successfully,
	 *         otherwise 1.
	 */
	int save_to_disk ();
	/**
	 * Load a saved hash from disk.
	 * \return zero is returned if the file is loaded successfully,
	 *         otherwise 1.
	 */
	int load_from_disk ();

    /**
     * Load a saved hash from disk.
     * \return returns actual path name.
     */
    char * get_actual_path_name();

private:
    char m_szActualPathName[MAX_PATH1];
};

#endif // PFCPERSISTENTCONFIG_H
