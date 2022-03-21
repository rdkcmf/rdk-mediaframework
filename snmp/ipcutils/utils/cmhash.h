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



/** \file pfchash.h General hash table system. */

#ifndef PFCHASH_H
#define PFCHASH_H

#include <stdint.h>
#ifdef GCC4_XXX
#undef max
#undef min
#include <tr1/unordered_map>
#else
#include <hash_map.h>
#endif

#ifndef PFCMUTEX_H
#include "rmf_osal_sync.h"
#endif

#ifndef CM_HASH_START_SIZE
#define CM_HASH_START_SIZE 127
#endif

#ifndef USE_STL_HASH_MAP

//! A struct to store the hash table entries
typedef struct CMHashNode
{
	char *key;
	char *value;
	struct CMHashNode *link;
} CMHashNode;

#else

//! A struct for STL hash_map key comparisons
struct eqstr
{
	/**
	 * Do a strcmp on both keys and return the result
	 * \param s1 the first hash key to compare
	 * \param s2 the second hash key to compare
	 */
	bool operator()(const char* s1, const char* s2) const
	{
		return strcmp(s1, s2) == 0;
	}
};

#endif


/** 
 * \brief General purpose hash table
 *
 * Simple hash mapping of string keys and multiple value types.
 * Keys and values are both copied and stored as strings.  The
 * entire hash can be saved or restored from file.
 *
 * If an alt hash is set, the alt hash is consulted if the key 
 * isn't found in this.
 */

class CMHash//:public pfcObject
{
  public:
	//! Init member vars and create a mutex for locking.
	CMHash ();
	/**
	 * Init member vars and create a mutex for locking.
	 * \param buckets the initial number of buckets to allocate in the hash table
	 */
	CMHash (int buckets);
	//! Frees mutex and hash data.
	virtual ~ CMHash ();

	/**
	 * Save the current hash to disk escaping special characters.
	 * \param path the the file in which the hash will be saved
	 * \return zero is returned if the file is saved successfully,
	 *         otherwise 1.
	 */
	int save_to_disk (char *path);
	/**
	 * Load a saved hash from disk.
	 * \param path the file to load tha hash from
	 * \return zero is returned if the file is loaded successfully,
	 *         otherwise 1.
	 */
	int load_from_disk (char *path);

	/**
	 * Read int value from the hash.
	 * \param key a string key to use for the hash lookup
	 * \param default_ a parameter to specify a int lookup and provide a default
	 *        return value
	 * \return the default_ parameter is returned if nothing is found in the hash
	 */
	int read (char *key, int default_);
	/**
	 * Read int value from the hash, interpreted as hex.
	 * \param key a string key to use for the hash lookup
	 * \param default_ a parameter to specify a hex lookup and provide a default
	 *        return value
	 * \return the default_ parameter is returned if nothing is found in the hash
	 */
	unsigned long read_hex (char *key, unsigned long default_);
	/**
	 * Read long value from the hash.
	 * \param key a string key to use for the hash lookup
	 * \param default_ a parameter to specify a long int lookup and provide a
	 *        default return value
	 * \return the default_ parameter is returned if nothing is found in the hash
	 */
	long read (char *key, long default_);
	/**
	 * Read double value from the hash.
	 * \param key a string key to use for the hash lookup
	 * \param default_ a parameter to specify a double lookup and provide a
	 *        default return value
	 * \return the default_ parameter is returned if nothing is found in the hash
	 */
	double read (char *key, double default_);
	/**
	 * Read float value from the hash.
	 * \param key a string key to use for the hash lookup
	 * \param default_ a parameter to specify a float lookup and provide a
	 *        default return value.
	 * \return the default_ parameter is returned if nothing is found in the hash.
	 */
	float read (char *key, float default_);
	/**
	 * Read 64 bit int value from the hash.
	 * \param key a string key to use for the hash lookup
	 * \param default_ a parameter to specify a 64 bit int lookup and provide a
	 *        default return value.
	 * \return the default_ parameter is returned if nothing is found in the hash.
	 */
	int64_t read (char *key, int64_t default_);
	/**
	 * Read size bytes from the hash into ptr.
	 * \param key a string key to use for the hash lookup
	 * \param ptr a pointer destination for the string copy from the hash data
	 * \param size the size of ptr
	 * \return the ptr is returned if nothing is found in the hash.
	 */
	char *read (char *key, char *ptr, size_t size);

    /**
     * Read size bytes from the hash into ptr.
     * Useful when load_from_disk() and write_to_disk() is NOT being used.
     * \param key a string key to use for the hash lookup
     * \param ptr a pointer destination for the buffer copy from the hash data
     * \param requested_bytes the number of bytes to copy
     * \param buffer_capacity the size of ptr
     * \return the ptr is returned if nothing is found in the hash.
     */
    
    void *read_fixed_buffer (char *key, void *ptr, size_t requested_bytes, size_t buffer_capacity);
    
	/**
	 * The string argument is not touched if the key can't be found.  Otherwise
	 * the value is copied into the string argument and the string argument returned.
	 * \param key a string key to use for the hash lookup
	 * \param string a pointer destination for the string copy from the hash data
	 * \return the string parameter is returned if nothing is found in the hash.
	 */
	char *read (char *key, char *string);
	/**
	 * Read a pointer value from the hash.
	 * \param key a string key to use for the hash lookup
	 * \param ptr a pointer to be used as the default return value
	 * \return the pointer that was stored in the hash or ptr if none was found
	 */
	void *read (char *key, void *ptr);
	/**
	 * Read a fixed point number and convert it to base.
	 * \param key a string key to use for the hash lookup
	 * \param default_ a parameter to specify a 64 bit int lookup and provide a
	 *        default return value.
	 * \param base the base of the fixed point number in the lookup
	 * \return the default_ parameter is returned if nothing is found in the hash.
	 */
	int64_t read_fixed (char *key, int64_t default_, int64_t base);

	/**
	 * Write an int value into the hash table.
	 * \param key a string to use for the hash key
	 * \param value the int value to write to the hash data string
	 */
	void write (char *key, int value);
    /**
     * Write an int value into the hash table, encoded in hex.
     * \param key a string to use for the hash key
     * \param value the int value to write to the hash data string
     */
    void write_hex (char *key, unsigned long value);
	/**
	 * Write a long value into the hash table.
	 * \param key a string to use for the hash key
	 * \param value the long int value to write to the hash data string
	 */
	void write (char *key, long value);
	/**
	 * Write a double value into the hash table.
	 * \param key a string to use for the hash key
	 * \param value the double value to write to the hash data string
	 */
	void write (char *key, double value);
	/** Write a float value into the hash table.
	 * \param key a string to use for the hash key
	 * \param value the float value to write to the hash data string
	 */
	void write (char *key, float value);
	/**
	 * Write a 64 bit int value into the hash table.
	 * \param key a string to use for the hash key
	 * \param value the 64 bit int value to write to the hash data string
	 */
	void write (char *key, int64_t value);
	/**
	 * Write a string into the hash table.
	 * \param key a string to use for the hash key
	 * \param value the string to copy to the hash data string
	 */
	void write (char *key, char *string);
	/**
	 * Write a pointer value into the hash table.
	 * \param key a string to use for the hash key
	 * \param ptr the pointer to write to the hash data string
	 */
	void write (char *key, void *ptr);

	/** Write a function pointer in to the hash table
        **/
	void write (char *key, void (*fnptr)(void*,uint32_t,void*));
	/**
	 * Write a fixed point number and convert it from base.
	 * \param key a string to use for the hash key
	 * \param value the 64 bit int to write to the hash data string
	 * \param base the base of the fixed point number to write
	 */
	void write_fixed (char *key, int64_t value, int64_t base);

	/**
	 * Copy specified data from ptr and store it under the key.
     * Caution: Writing un-printable characters will corrupt the config stored on disk.
     *          Do not store un-printable characters including '\0' when using save_to_disk()
     *          and load_from_disk() API.
     * \param key a string to use for the hash key
	 * \param ptr the pointer to data to be written to the table
	 * \param size the size of the data to write to the table
	 */
	void write (char *key, void *ptr, size_t size);


	/**
	 * Delete the node associated with key from the hash
	 * \param key the string key for the hash entry to remove
	 */
	void remove (char *key);


	//! Return the number of keys in the hash table.
	int get_size ();

	/**
	 * Sets an alternate hash table to read if the key isn't in the current one.
	 * Used for layouts where themes override themes.
	 * \param hash the alternate hash to use in lookups
	 */
	void set_alt_hash (CMHash * hash);

	/** For debugging. Prints hash table to \e stdout.
	 * \param tab the number of spaces to display before each line in the dump
	 */
	void dump (int tab = 0);

  private:
	//! The alternate hash to use if the key is not found in this table
	CMHash *alt_hash;
	//! The lock for hash access
	rmf_osal_Mutex hash_lock;
	//! The lock for saving to file and restoring from file
	rmf_osal_Mutex file_lock;
	//! The count of keys in the database
	int keys;
	/**
	 * Read the pointer straight from the hash node.  No string conversions.
	 * \param key the key to look up in the hash table
	 * \return a pointer to the value associated with \a key
	 */
	char **get_value (char *key);
	/**
	 * Store the \a key and \a ptr to a value string in the hash
	 * \param key the string key for the key/value pair to store in the hash
	 * \param ptr the string value to store in the hash
	 */
	void set_value (char *key, char *ptr);

#ifndef USE_STL_HASH_MAP
	/**
	 * Hashing function
	 * \param key the string to use in creating the hash number
	 */
	int hash(char *key);
    /**
     * Find the node associated with the string key in the hash table
     * \param key the string to use for the hash lookup
     * \return the node containing the string \a key
     */
    CMHashNode *find_node(char *key);

	//! The pointer to the linked list buckets of the hash table
	CMHashNode **nodes;
	//! The number of buckets in the hash table
	int buckets;
#else
	//! The STL hash_map to use for the hash table
	hash_map<char *, char *, hash<char *>, eqstr> *hmap;
#endif
};

#endif
