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
#ifndef _PFC_ENV_H_
#define _PFC_ENV_H_

#include <stdlib.h>
#include <stdarg.h>

/**
 * Utility functions to get a configuration value from the environment.
 *
 * Currently this is the file "/mnt/nfs/env/rmfconfig.ini" by calling <code>mpeos_envGet</code>
 */


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get a configuration value as a string.
 *
 * @param result This value is returned if no configuration is found. May be NULL.
 * @param key The configuration key string.
 * @return the configuration value, or the backup result if not found.
 */
const char *vl_env_get_str(const char *result, const char *key);

/**
 * Get a configuration value as a string.
 *
 * @param result This value is returned if no configuration is found. May be NULL.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return the configuration value, or the backup result if not found.
 */
const char *vl_env_vget_str(const char *result, const char *key, ...);


/**
 * Test a configuration value for equivalence with a string.
 *
 * @param test The string to compare the configuration value against.
 * @param key The configuration key string.
 * @return true if the configuration value exists and is the same as the test string.
 */
int vl_env_compare(const char *test, const char *key);

/**
 * Test a configuration value for equivalence with a string.
 *
 * @param test The string to compare the configuration value against.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return true if the configuration value exists and is the same as the test string.
 */
int vl_env_vcompare(const char *test, const char *key, ...);


/**
 * Test if configuration value is true.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string.
 * @return true if the configuration value exists and is set to the string "TRUE"
 */
int vl_env_get_bool(int result, const char *key);

/**
 * Test if configuration value is true.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return true if the configuration value exists and is set to the string "TRUE"
 */
int vl_env_vget_bool(int result, const char *key, ...);


/**
 * Get a configuration value as an integer.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string.
 * @return the configuration value, or the backup result if not found.
 */
int vl_env_get_int(int result, const char *key);

/**
 * Get a configuration value as an integer.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return the configuration value, or the backup result if not found.
 */
int vl_env_vget_int(int result, const char *key, ...);


/**
 * Get a hexadecimal configuration value as an integer.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string.
 * @return the configuration value, or the backup result if not found.
 */
int vl_env_get_hex(int result, const char *key);

/**
 * Get a hexadecimal configuration value as an integer.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return the configuration value, or the backup result if not found.
 */
int vl_env_vget_hex(int result, const char *key, ...);


/**
 * Get a configuration value as a single-precision float.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string.
 * @return the configuration value, or the backup result if not found.
 */
float vl_env_get_float(float result, const char *key);

/**
 * Get a configuration value as a single-precision float.
 *
 * @param result This value is returned if no configuration is found.
 * @param key The configuration key string. May be a format string as defined by
 *        the standard library <code>printf()</code> function.
 * @return the configuration value, or the backup result if not found.
 */
float vl_env_vget_float(float result, const char *key, ...);


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
int vl_env_get_enum(int result, const char **names, int *results, const char *key);

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
int vl_env_vget_enum(int result, const char **names, int *results, const char *key, ...);


unsigned char vl_isFeatureEnabled( unsigned char *featureString );

int vl_GetEnvValue( unsigned char *featureString );

float vl_GetEnvValue_float( unsigned char *featureString );

#ifdef __cplusplus
}
#endif

#endif // _PFC_ENV_H_
