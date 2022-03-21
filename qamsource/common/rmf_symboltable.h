/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2013 RDK Management
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


/**
 * @file rmf_symboltable.h
 * @brief File contains the symbol table information.
 */


/**
 * @defgroup symboltable Symbol Table
 * @ingroup Inband
 * @defgroup symboltableclass Symbol Table Class
 * @ingroup symboltable
 * @defgroup symboltabledatastructures Symbol Table Data Structures
 * @ingroup symboltable
 */


typedef unsigned int rmf_SymbolMapKey;

typedef struct node_s
{
        rmf_SymbolMapKey  key;
        void            *data;
        struct node_s   *next;
}node_t;

typedef node_t* rmf_SymbolMapTableIter;


/**
 * @class rmf_SymbolMapTable
 * @brief This class used to manipulate the key and data.
 * For example, we can insert the data-key pair, remove it by searching a specifi key etc.
 * @ingroup symboltableclass
 */
class rmf_SymbolMapTable
{
private:
        node_t  *head;

public:
	rmf_SymbolMapTable();	
	~rmf_SymbolMapTable();

	void Insert(rmf_SymbolMapKey key, void *data);
	void* Lookup(rmf_SymbolMapKey key);
	void* Remove(rmf_SymbolMapKey key);

	void IterInit(rmf_SymbolMapTableIter *iter);
	void* IterNext(rmf_SymbolMapTableIter *iter, rmf_SymbolMapKey *key);
};

