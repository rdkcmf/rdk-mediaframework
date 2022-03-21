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

#include <rdk_debug.h>
#include <rmf_osal_util.h>
#include <string.h>

#include "vlEnv.h"

#define BUFFER_SIZE 64


/**
 * Get a configuration value as a string.
 *
 * @param result This value is returned if no configuration is found. May be NULL.
 * @param key The configuration key string.
 * @return the configuration value, or the backup result if not found.
 */
const char *vl_env_get_str(const char *result, const char *key)
{
    const char *value = rmf_osal_envGet(key);

    if (value != NULL)
    {
        result = value;
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, value=%s", __FUNCTION__, key, result);

    return result;
}


/**
 * Get a configuration value as a string.
 *
 * @param result This value is returned if no configuration is found. May be NULL.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return the configuration value, or the backup result if not found.
 */
const char *vl_env_vget_str(const char *result, const char *key, ...)
{
    char fmt_key[BUFFER_SIZE];

    va_list values;
    va_start(values, key);

    vsnprintf(fmt_key, BUFFER_SIZE, key, values);

    va_end(values);

    const char *value = rmf_osal_envGet(fmt_key);

    if (value != NULL)
    {
        result = value;
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, value=%s", __FUNCTION__, fmt_key, result);

    return result;
}


/**
 * Test a configuration value for equivalence with a string.
 *
 * @param test The string to compare the configuration value against.
 * @param key The configuration key string.
 * @return true if the configuration value exists and is the same as the test string.
 */
int vl_env_compare(const char *test, const char *key)
{
    const char *value = rmf_osal_envGet(key);

    bool result = ((value != NULL) && (strcmp(value, test) == 0));

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, test=%s, result=%d",
            __FUNCTION__, key, test, result);

    return result;
}


/**
 * Test a configuration value for equivalence with a string.
 *
 * @param test The string to compare the configuration value against.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return true if the configuration value exists and is the same as the test string.
 */
int vl_env_vcompare(const char *test, const char *key, ...)
{
    char fmt_key[BUFFER_SIZE];

    va_list values;
    va_start(values, key);

    vsnprintf(fmt_key, BUFFER_SIZE, key, values);

    va_end(values);

    const char *value = rmf_osal_envGet(fmt_key);

    bool result = ((value != NULL) && (strcmp(value, test) == 0));

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, test=%s, result=%d",
            __FUNCTION__, fmt_key, test, result);

    return result;
}


/**
 * Test if configuration value is true.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string.
 * @return true if the configuration value exists and is set to the string "TRUE"
 */
int vl_env_get_bool(int result, const char *key)
{
    const char *value = rmf_osal_envGet(key);

	if (value != NULL)
	{
        result = (strcasecmp(value, "TRUE") == 0);
	}

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, result=%d", __FUNCTION__, key, result);

    return result;
}


/**
 * Test if configuration value is true.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return true if the configuration value exists and is set to the string "TRUE"
 */
int vl_env_vget_bool(int result, const char *key, ...)
{
    char fmt_key[BUFFER_SIZE];

    va_list values;
    va_start(values, key);

    vsnprintf(fmt_key, BUFFER_SIZE, key, values);

    va_end(values);

    const char *value = rmf_osal_envGet(fmt_key);

	if (value != NULL)
	{
        result = (strcasecmp(value, "TRUE") == 0);
	}


    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, result=%d", __FUNCTION__, fmt_key, result);

    return result;
}


/**
 * Get a configuration value as an integer.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string.
 * @return the configuration value, or the backup result if not found.
 */
int vl_env_get_int(int result, const char *key)
{
    const char *value = rmf_osal_envGet(key);

    if (value != NULL)
    {
        result = atoi(value);
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, value=%d", __FUNCTION__, key, result);

    return result;
}


/**
 * Get a configuration value as an integer.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return the configuration value, or the backup result if not found.
 */
int vl_env_vget_int(int result, const char *key, ...)
{
    char fmt_key[BUFFER_SIZE];

    va_list values;
    va_start(values, key);

    vsnprintf(fmt_key, BUFFER_SIZE, key, values);

    va_end(values);

    const char *value = rmf_osal_envGet(fmt_key);

    if (value != NULL)
    {
        result = atoi(value);
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, value=%d", __FUNCTION__, fmt_key, result);

    return result;
}


/**
 * Get a hexadecimal configuration value as an integer.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string.
 * @return the configuration value, or the backup result if not found.
 */
int vl_env_get_hex(int result, const char *key)
{
    const char *value = rmf_osal_envGet(key);

    if (value != NULL)
    {
        result = (int) strtol(value, NULL, 16);
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, value=%d", __FUNCTION__, key, result);

    return result;
}


/**
 * Get a hexadecimal configuration value as an integer.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return the configuration value, or the backup result if not found.
 */
int vl_env_vget_hex(int result, const char *key, ...)
{
    char fmt_key[BUFFER_SIZE];

    va_list values;
    va_start(values, key);

    vsnprintf(fmt_key, BUFFER_SIZE, key, values);

    va_end(values);

    const char *value = rmf_osal_envGet(fmt_key);

    if (value != NULL)
    {
        result = (int) strtol(value, NULL, 16);
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, value=%d", __FUNCTION__, fmt_key, result);

    return result;
}


/**
 * Get a configuration value as a single-precision float.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string.
 * @return the configuration value, or the backup result if not found.
 */
float vl_env_get_float(float result, const char *key)
{
    const char *value = rmf_osal_envGet(key);

    if (value != NULL)
    {
        result = atof(value);
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, value=%f", __FUNCTION__, key, result);

    return result;
}


/**
 * Get a configuration value as a single-precision float.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return the configuration value, or the backup result if not found.
 */
float vl_env_vget_float(float result, const char *key, ...)
{
    char fmt_key[BUFFER_SIZE];

    va_list values;
    va_start(values, key);

    vsnprintf(fmt_key, BUFFER_SIZE, key, values);

    va_end(values);

    const char *value = rmf_osal_envGet(fmt_key);

    if (value != NULL)
    {
        result = atof(value);
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, value=%f", __FUNCTION__, fmt_key, result);

    return result;
}


/**
 * Get a configuration value as an enumeration value based on a list of strings (names) and
 * a list of ints (results).
 *
 * The name list comparisons are case-insensitive.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @param names A list of names which correspond with the results list. The last entry must be NULL.
 * @param results A list of values which correspond with the names list.
 * @return the configuration value, or the backup result if not found.
 */
int vl_env_get_enum(int result, const char **names, int *results, const char *key)
{
    const char *value = rmf_osal_envGet(key);

    if (value != NULL)
    {
		while (*names != NULL)
		{
			if (strcasecmp(value, *names) == 0)
			{
				result = *results;
				break;
			}

			names++;
			results++;
		}
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, value=%d", __FUNCTION__, key, result);

    return result;
}


/**
 * Get a configuration value as an enumeration value based on a list of strings (names) and
 * a list of ints (results).
 *
 * The name list comparisons are case-insensitive.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @param names A list of names which correspond with the results list. The last entry must be NULL.
 * @param results A list of values which correspond with the names list.
 * @return the configuration value, or the backup result if not found.
 */
int vl_env_vget_enum(int result, const char **names, int *results, const char *key, ...)
{
    char fmt_key[BUFFER_SIZE];

    va_list values;
    va_start(values, key);

    vsnprintf(fmt_key, BUFFER_SIZE, key, values);

    va_end(values);

    const char *value = rmf_osal_envGet(fmt_key);

    if (value != NULL)
    {
		while (*names != NULL)
		{
			if (strcasecmp(value, *names) == 0)
			{
				result = *results;
				break;
			}

			names++;
			results++;
		}
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, value=%d", __FUNCTION__, fmt_key, result);

    return result;
}

