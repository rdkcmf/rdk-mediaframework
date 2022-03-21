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





#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <values.h>
#include <unistd.h>
#include <stdlib.h>
#include "cmhash.h"
#include <string.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif
/**
 * For C style strings
 */
#define CM_HASH_TEXT_LEN 1024

CMHash::CMHash ()
{
    hash_lock = 0;
	rmf_osal_mutexNew(&hash_lock);
	rmf_osal_mutexNew(&file_lock);
	alt_hash = 0;

#ifndef USE_STL_HASH_MAP

	buckets = CM_HASH_START_SIZE;
	keys = 0;
	nodes = new CMHashNode*[CM_HASH_START_SIZE];
	for(int i = 0; i < CM_HASH_START_SIZE; i++) {
		nodes[i] = NULL;
	}
#else

	hmap = new hash_map<char *, char *, hash<char *>, eqstr>(CM_HASH_START_SIZE);

#endif
}

CMHash::CMHash (int size)//:pfcObject ()
{
       file_lock = 0;
	alt_hash = 0;
	hash_lock = 0;
	rmf_osal_mutexNew(&hash_lock);
	rmf_osal_mutexNew(&file_lock);
  
#ifndef USE_STL_HASH_MAP

	buckets = size;
	keys = 0;
	nodes = new CMHashNode*[size];
	for(int i = 0; i < size; i++) {
		nodes[i] = NULL;
	}

#else

	hmap = new hash_map<char *, char *, hash<char *>, eqstr>(size);

#endif
}

CMHash::~CMHash ()
{
       rmf_osal_mutexDelete(hash_lock);
	rmf_osal_mutexDelete(file_lock);

#ifndef USE_STL_HASH_MAP

	for (int i = 0; i < buckets; i++)
	{
		for (CMHashNode *node = nodes[i]; node != 0; node = nodes[i])
		{
			delete[](char *)node->value;
			delete[]node->key;
			nodes[i] = node->link;
			delete node;
		}
	}
	delete[]nodes;

#else

	hash_map<char *, char *, hash<char *>, eqstr>::iterator i;
	for(i = hmap->begin(); i != hmap->end(); i++)
	{
		delete[]i->first;
		delete[]i->second;
	}
	delete hmap;

#endif
}

int
CMHash::load_from_disk (char *path)
{
	rmf_osal_mutexAcquire(file_lock);
	//  RDK_LOG(RDK_LOG_INFO,"LOG.RDK.OS","CMHash::load_from_disk 1 %s\n", path);
	FILE *file = fopen (path, "r");

	//  RDK_LOG(RDK_LOG_INFO,"LOG.RDK.OS","CMHash::load_from_disk 2\n");
	if (!file)
	{
//         fprintf (stderr, "CMHash::load_from_disk(%s) : error %s\n", path, strerror (errno));
		rmf_osal_mutexRelease(file_lock);
		return 1;
	}

	char string[(CM_HASH_TEXT_LEN * 2) + 1];
	char key[CM_HASH_TEXT_LEN];
	char value[CM_HASH_TEXT_LEN];
	//  RDK_LOG(RDK_LOG_INFO,"LOG.RDK.OS","CMHash::load_from_disk 3\n");
	while (!(feof (file) || ferror(file)))
	{
		char *buf_ptr = key;
		char *str_ptr = string;
		bool key_done = 0;
        // Aug-06-2009: Added bounds tracker
        int  nLength = 0;
		string[0] = '\0';
		value[0] = '\0';

		fgets (string, CM_HASH_TEXT_LEN * sizeof(char), file);

		if (string[0] != '\0')
		{
			while (*str_ptr != '\n' && *str_ptr != '\0')
			{
				if(*str_ptr == '=')
				{
					if(!key_done) {
						*buf_ptr = '\0';
						buf_ptr = value;
						str_ptr++;
						key_done = 1;
                        // Aug-06-2009: Reset bounds tracker
                        nLength = 0;
						continue;
					}
					else {
// 						fprintf(stderr, "CMHash::load_from_disk(%s) unescaped = in value\n", path);

                        if(file != NULL)
                        {
                            fclose(file);
                        }
                        
		rmf_osal_mutexRelease(file_lock);
						return 1;
					}
				}
				// use escaped n and escaped 0 to mean \n and \0
				if(*str_ptr == '\\' && (str_ptr < (string + CM_HASH_TEXT_LEN - 2)))
				{
					str_ptr++;
					if(*str_ptr == '\0') {
//                         fprintf(stderr, "CMHash::load_from_disk(%s) file corruption\n", path);

                        if(file != NULL)
                        {
                            fclose(file);
                        }
                        
		rmf_osal_mutexRelease(file_lock);
                        return 1;
					}
					else if(*str_ptr == 'n')
					{
						*buf_ptr++ = '\n';
						str_ptr++;
						continue;
					}
				}
                
                // Aug-06-2009: Added missing bounds check
                if(nLength < (CM_HASH_TEXT_LEN-1))
                {
                    *buf_ptr++ = *str_ptr++;
                    // Aug-06-2009: Added default termination for every character added to string
                    *buf_ptr = '\0';
                    // Aug-06-2009: Increment bounds tracker
                    nLength++;
                }
			}
            // Aug-06-2009: Observation: The following if block may no-op in out-of-bounds cases,
            //                           however the strings are guaranteed to be terminated
            //                           by the bounds logic tracked by the nLength variable.
            if(key_done && buf_ptr < (value + CM_HASH_TEXT_LEN - 1)) {
				*buf_ptr = '\0';
			}
			else {
//                 fprintf(stderr, "CMHash::load_from_disk(%s) error parsing value\n", path);

                if(file != NULL)
                {
                    fclose(file);
                }

		rmf_osal_mutexRelease(file_lock);
                return 1;
			}

			write (key, value);
		}
	}

	//  RDK_LOG(RDK_LOG_INFO,"LOG.RDK.OS","CMHash::load_from_disk 4\n");
	fclose (file);
	rmf_osal_mutexRelease(file_lock);

	//  RDK_LOG(RDK_LOG_INFO,"LOG.RDK.OS","CMHash::load_from_disk 5\n");

	return 0;
}

int
CMHash::save_to_disk (char *path)
{
	//  RDK_LOG(RDK_LOG_INFO,"LOG.RDK.OS","CMHash::save_to_disk 1 %s\n", path);
	rmf_osal_mutexAcquire(file_lock);
	rmf_osal_mutexAcquire(hash_lock);
	FILE *file = fopen (path, "w");
	char *str_ptr;
	char *line_ptr;
	char c;
	bool key_done;
    
	if (!file)
	{
// 		fprintf (stderr, "CMHash::save_to_disk(%s) : error %s", path, strerror (errno));
		rmf_osal_mutexRelease(hash_lock);
		rmf_osal_mutexRelease(file_lock);
		return 1;
	}

#ifndef USE_STL_HASH_MAP

	for (int i = 0; i < buckets; i++)
	{
		for(CMHashNode *node = nodes[i]; node != NULL; node = node->link)
		{
			line_ptr = str_ptr = node->key;

#else

	hash_map<char *, char *, hash<char *>, eqstr>::iterator i;
	for (i = hmap->begin(); i != hmap->end(); i++)
	{
			line_ptr = str_ptr = i->first;

#endif
			key_done = 0;

			while(1)
			{
				if(*str_ptr == '\0')
				{
					fputs(line_ptr, file);
					if(!key_done)
					{
						key_done = 1;
						fputc('=', file);

#ifndef USE_STL_HASH_MAP

						line_ptr = str_ptr = node->value;

#else

						line_ptr = str_ptr = i->second;

#endif
						continue;
					}
					else
					{
						fputc('\n', file);
						break;
					}
				}
				else if(*str_ptr == '\n' || *str_ptr == '\\' || *str_ptr == '=')
				{
					c = *str_ptr;
					*str_ptr = '\0';
					fputs(line_ptr, file);
					*str_ptr = c;
					line_ptr = &(str_ptr[1]);
					fputc('\\', file);
					if(c == '\n') {
						fputc('n', file);
					}
					else {
						fputc(c, file);
					}
				}
				str_ptr++;
			}
#ifndef USE_STL_HASH_MAP

		}
	}

#else

    }

#endif

    fflush (file); // MAR-09-2011 : Flush file buffers to filesystem memory
    fsync(fileno(file)); // MAR-09-2011 : Flush file from filesystem memory to HDD
	fclose (file);
	rmf_osal_mutexRelease(hash_lock);
	rmf_osal_mutexRelease(file_lock);

	return 0;
}

void
CMHash::dump (int tab)
{

// 	for (int i = 0; i < tab; i++)
// 		putchar (' ');

// 	puts ("CMHash::dump\n");

#ifndef USE_STL_HASH_MAP

	for (int i = 0; i < buckets; i++)
	{

#else

	hash_map<char *, char *, hash<char *>, eqstr>::iterator i;
	for(i = hmap->begin(); i != hmap->end(); i++) {

#endif

// 		for (int j = 0; j < tab; j++)
// 			putchar (' ');

#ifndef USE_STL_HASH_MAP

		for(CMHashNode *node = nodes[i]; node != 0; node = node->link)
		{
		}

#else


#endif

	}
	rmf_osal_mutexRelease(hash_lock);
	if (alt_hash)
	{
		for (int i = 0; i < tab; i++)
			putchar (' ');
// 		puts ("CMHash::dump alternate\n");
		alt_hash->dump (tab + 1);
	}
}

#ifndef USE_STL_HASH_MAP

int
CMHash::hash(char *key)
{
	enum { LSHIFT = 4 };
	enum { RSHIFT = LONGBITS - LSHIFT - 1 };
	enum { MASK = ~ ( 1LU << ( LONGBITS - 1 )) };
	unsigned long hash = 0;
	for( ; *key != '\0'; key++ ) {
		hash = ( hash << LSHIFT ) + ( hash >> RSHIFT ) + *key;
	}
	return (int)((hash & MASK) % buckets);
}

CMHashNode *
CMHash::find_node (char *key)
{
    // The lock should be acquired while in this function
    CMHashNode *node;
    for(node = nodes[hash(key)]; node != NULL; node = node->link)
    {
        if(!strcmp(key, node->key)) {
            break;
        }
    }
    return node;
}

void
CMHash::set_value (char *key, char *ptr)
{
	rmf_osal_mutexAcquire(hash_lock);

	CMHashNode *node = find_node(key);

	if(node != NULL) {
		delete[] node->value;
	}
	else {
		int i = hash(key);
		node = nodes[i];
		if(node == NULL) {
			node = nodes[i] = new CMHashNode;
		}
		else {
			for(CMHashNode *next = node->link; next != NULL; next = node->link)
			{
				node = next;
			}
			node->link = new CMHashNode;
			node = node->link;
		}
		node->link = NULL;
		i = strlen(key) + 1;
		node->key = new char[i];
		//memcpy(node->key, key, i);
		memcpy(node->key, key, i);
		keys++;
	}

	node->value = ptr;

	rmf_osal_mutexRelease(hash_lock);
}

char **
CMHash::get_value (char *key)
{
	// The lock should be acquired while in this function
	CMHashNode *node = find_node(key);
	if(node != NULL)
	{
		return &(node->value);
	}
	return NULL;
}

void
CMHash::remove (char *key)
{
	CMHashNode *node, *last;
    int i = hash(key);
	rmf_osal_mutexAcquire(hash_lock);

	for(last = NULL, node = nodes[i]; node != NULL; last = node, node = node->link)
	{
		if(!strcmp(key, node->key)) {
            if(last != NULL) {
                last->link = node->link;
            }
            else {
                nodes[i] = node->link;
            }
            delete[] node->key;
            delete[] node->value;
            delete node;
			break;
		}
	}
	rmf_osal_mutexRelease(hash_lock);
}

#else

void
CMHash::set_value (char *key, char *val)
{
	hash_lock->lock ("CMHash::set_value");

	hash_map<char *, char *, hash<char *>, eqstr>::iterator i;
	i = hmap->find(key);
	if(i != hmap->end()) {
		delete[]i->second;
        i->second = val;
	}
	else {
        size_t len = strlen(key);
        char *new_key = new char[len + 1];
        strncpy (new_key, key, len);
        new_key[len] = '\0';
		hmap->insert(pair<char *, char *>(new_key, val));
	}

	rmf_osal_mutexRelease(hash_lock);
}

char **
CMHash::get_value (char *key)
{
	hash_map<char *, char *, hash<char *>, eqstr>::iterator i;
	i = hmap->find(key);
	if(i != hmap->end())
	{
		return &(i->second);
	}
	return (char **)NULL;
}

void
CMHash::remove (char *key)
{
	hash_lock->lock ("CMHash::remove");
	hash_map<char *, char *, hash<char *>, eqstr>::iterator i;
	i = hmap->find(key);
	if(i != hmap->end()) {
        char *val = i->second;
        key = i->first;
        hmap->erase(i);
        delete[] val;
        delete[] key;
	}
	rmf_osal_mutexRelease(hash_lock);
}

#endif

int
CMHash::read (char *key, int default_)
{
	rmf_osal_mutexAcquire(hash_lock);
	char **vptr = get_value(key);
	if (vptr != NULL)
	{
		default_ = atol ((char *)*vptr);
	}
	else if (alt_hash)
	{
		default_ = alt_hash->read (key, default_);
	}

	rmf_osal_mutexRelease(hash_lock);
	return default_;
}

int64_t
CMHash::read (char *key, int64_t default_)
{
	rmf_osal_mutexAcquire(hash_lock);
	char **vptr = get_value(key);
	if (vptr != NULL)
	{
		sscanf ((char *)*vptr, "%lld", &default_);
	}
	else if (alt_hash)
	{
		default_ = alt_hash->read (key, default_);
	}

	rmf_osal_mutexRelease(hash_lock);
	return default_;
}

unsigned long
CMHash::read_hex (char *key, unsigned long default_)
{
	rmf_osal_mutexAcquire(hash_lock);
	char **vptr = get_value(key);
	if (vptr != NULL)
	{
		sscanf ((char *)*vptr, "%x", &default_);
	}
	else if (alt_hash)
	{
		default_ = alt_hash->read_hex (key, default_);
	}
	rmf_osal_mutexRelease(hash_lock);
	return default_;
}

long
CMHash::read (char *key, long default_)
{
	rmf_osal_mutexAcquire(hash_lock);
	char **vptr = get_value(key);
	if (vptr != NULL)
	{
		default_ = atol ((char *)*vptr);
	}
	else if (alt_hash)
	{
		default_ = alt_hash->read (key, default_);
	}
	rmf_osal_mutexRelease(hash_lock);
	return default_;
}

int64_t
CMHash::read_fixed (char *key, int64_t default_, int64_t base)
{
#if 0
	hash_lock->lock ("CMHash::read_fixed");
	char **vptr = get_value(key);
	if (vptr != NULL)
	{
		default_ = pfcUtil::atofixed ((char *)*vptr, base);
	}
	else if (alt_hash)
	{
		default_ = alt_hash->read_fixed (key, default_, base);
	}
	rmf_osal_mutexRelease(hash_lock);
#endif	
	return default_;
}

double
CMHash::read (char *key, double default_)
{
// 	fprintf (stderr, "CMHash::read %s: double not reliable on MIPS.\n", key);
	rmf_osal_mutexAcquire(hash_lock);
	char **vptr = get_value(key);
	if (vptr != NULL)
	{
		default_ = atof ((char *)*vptr);
	}
	else if (alt_hash)
	{
		default_ = alt_hash->read (key, default_);
	}
	rmf_osal_mutexRelease(hash_lock);
	return default_;
}

float
CMHash::read (char *key, float default_)
{
// 	fprintf (stderr, "CMHash::read %s: float not reliable on MIPS.\n", key);
	rmf_osal_mutexAcquire(hash_lock);
	char **vptr = get_value(key);
	if (vptr != NULL)
	{
		default_ = atof ((char *)*vptr);
	}
	else if (alt_hash)
	{
		default_ = alt_hash->read (key, default_);
	}
	rmf_osal_mutexRelease(hash_lock);
	return default_;
}

// The string argument is not touched if the key can't be found.  Otherwise
// the value is copied into the string argument and the string argument returned.
char *
CMHash::read (char *key, char *string)
{
	rmf_osal_mutexAcquire(hash_lock);
	char **vptr = get_value(key);
	if (vptr != NULL)
	{
/*		strcpy (string, *vptr);*/
		strcpy (string, *vptr);
		
	}
	else if (alt_hash)
	{
		alt_hash->read (key, string);
	}
	rmf_osal_mutexRelease(hash_lock);
	return string;
}

void *
CMHash::read (char *key, void *ptr)
{
	rmf_osal_mutexAcquire(hash_lock);
	char **vptr = get_value(key);
	if (vptr != NULL)
	{
		sscanf ((char *)*vptr, "%p", &ptr);
	}
	else if (alt_hash)
	{
		ptr = alt_hash->read (key, ptr);
	}
	rmf_osal_mutexRelease(hash_lock);
	return ptr;
}

char *
CMHash::read (char *key, char *ptr, size_t size)
{
	rmf_osal_mutexAcquire(hash_lock);
	char **vptr = get_value(key);
	if (vptr != NULL)
	{
        // Aug-06-2009: Changed to memcpy() to vlStrCpy() to avoid access violation.
        // Observation: Buffers with embedded '\0' cannot be stored.
        //            : Even if stored they are meaningless when retrieved.
        //            : This is because the actual length of the stored data cannot be inferred.
        strcpy(ptr, *vptr);
    }
	else if (alt_hash)
	{
		rmf_osal_mutexRelease(hash_lock);
		return alt_hash->read(key, ptr, size);
	}
	rmf_osal_mutexRelease(hash_lock);
	return ptr;
}

void *
CMHash::read_fixed_buffer (char *key, void *ptr, size_t requested_bytes, size_t buffer_capacity)
{// Useful when load_from_disk() and write_to_disk() is NOT being used.

    rmf_osal_mutexAcquire(hash_lock);
    char **vptr = get_value(key);
    if (vptr != NULL)
    {
        memcpy(ptr, *vptr, requested_bytes);
    }
    else if (alt_hash)
    {
        rmf_osal_mutexRelease(hash_lock);
        return alt_hash->read_fixed_buffer(key, ptr, requested_bytes, buffer_capacity);
    }
    rmf_osal_mutexRelease(hash_lock);
    return ptr;
}

void
CMHash::write (char *key, int value)
{
	char string[CM_HASH_TEXT_LEN];
    snprintf (string, sizeof(string), "%d", value);
	write (key, string);
}

void
CMHash::write_hex (char *key, unsigned long value)
{
    char string[CM_HASH_TEXT_LEN];
    snprintf (string, sizeof(string), "%x", value);
    write (key, string);
}
    
void
CMHash::write (char *key, int64_t value)
{
	char string[CM_HASH_TEXT_LEN];
	snprintf (string,sizeof(string), "%lld", value);
	write (key, string);
}

void
CMHash::write (char *key, long value)
{
	char string[CM_HASH_TEXT_LEN];
	snprintf (string, sizeof(string),"%ld", value);
	write (key, string);
}

void
CMHash::write (char *key, double value)
{
	// fprintf(stderr, "CMHash::write: double not reliable on MIPS.\n");
	char string[CM_HASH_TEXT_LEN];
	snprintf (string, sizeof(string),"%.16e", value);
	write (key, string);
}

void
CMHash::write (char *key, float value)
{
	// fprintf(stderr, "CMHash::write: float not reliable on MIPS.\n");
	char string[CM_HASH_TEXT_LEN];
	snprintf (string, sizeof(string),"%f", value);
	write (key, string);
}

void
CMHash::write (char *key, void *ptr)
{
	char string[CM_HASH_TEXT_LEN];
	snprintf (string, sizeof(string),"%p", ptr);
	write (key, string);
}

void
CMHash::write (char *key, void (*fnptr)(void *,uint32_t,void *))
{
	char string[CM_HASH_TEXT_LEN];
	snprintf (string, sizeof(string),"%p", fnptr);
	write (key, string);
}
void
CMHash::write_fixed (char *key, int64_t value, int64_t base)
{
	char string[CM_HASH_TEXT_LEN];
//	write (key, pfcUtil::fixedtoa (string, value, base));
}

void
CMHash::write (char *key, char *string)
{
	write(key, (void *)string, strlen (string) + 1);
}

void
CMHash::write (char *key, void *ptr, size_t size)
{
    // Caution: Writing un-printable characters will corrupt the config stored on disk.
    //          Do not store un-printable characters including '\0' when using save_to_disk()
    //          and load_from_disk() API.
    // Aug-06-2009: Added +1 to size.
    void *new_ptr = new char[size+1];
    // Aug-06-2009: NULL check
    if(NULL != new_ptr)
    {
        // Aug-06-2009: Added extra string termination.
        ((char*)new_ptr)[size] = '\0';
        memcpy(new_ptr, ptr, size);
        set_value(key, (char *)new_ptr);
    }
}

void
CMHash::set_alt_hash (CMHash * hash)
{
    if(alt_hash != this) {
	    this->alt_hash = hash;
    }
    else {
        //fprintf(stderr, "CMHash::set_alt_hash: called with hash this\n");
    }
}

int
CMHash::get_size ()
{
	return keys;
}

