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

/**
 * @file rmf_oobsimanagerstub.cpp
 */

#include"rmf_oobsimanagerstub.h"
#include <iostream>
#include "tinyxml.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

using namespace std;

#define XML_DECLARATION "1.0", "", ""
#define XML_ELEMENT_1STLEVEL "SIDB"
#define XML_ELEMENT_GROUP
#define XML_PGM_ELEMENT_NAME "program"
#define XML_SOURCE_ID_KEY "sourceid"
#define XML_FREQ_KEY "carrier_frequency"
#define XML_MODULATION_KEY "modulation_mode"
#define XML_SYMBOLRATE_KEY "symbol_rate"
#define XML_PGM_NO_KEY "program_number"
#define XML_NO_OF_ATRIBUTES 4

struct program_info_list_s
{
	uint32_t sourceID;
	rmf_ProgramInfo_t info;
	program_info_list_s* next;
};

static program_info_list_s* g_program_info_list = NULL;


const unsigned int NUM_INDENTS_PER_SPACE=2;


int xml_parse_attributes(TiXmlElement* pElement)
{
	if ( !pElement ) return 0;

	TiXmlAttribute* pAttrib=pElement->FirstAttribute();
	int i=0;
	int ival;
	rmf_ProgramInfo_t info = {};
	uint32_t source_id = 0;
	while (pAttrib)
	{
		//printf( "%s: value=[%s]\n",  pAttrib->Name(), pAttrib->Value());
		if (pAttrib->QueryIntValue(&ival)==TIXML_SUCCESS) 
		{
			if ( 0 == strcmp(pAttrib->Name(), XML_SOURCE_ID_KEY))
			{
				source_id = ival;
			}
			else if ( 0 == strcmp(pAttrib->Name(), XML_FREQ_KEY))
			{
				info.carrier_frequency = ival;
			}
			else if ( 0 == strcmp(pAttrib->Name(), XML_MODULATION_KEY))
			{
				info.modulation_mode = (rmf_ModulationMode)ival;
			}
			else if ( 0 == strcmp(pAttrib->Name(), XML_SYMBOLRATE_KEY))
			{
				info.symbol_rate = ival;
			}
			else if ( 0 == strcmp(pAttrib->Name(), XML_PGM_NO_KEY))
			{
				info.prog_num= ival;
			}
		}
		i++;
		pAttrib=pAttrib->Next();
	}
	if (source_id)//(i == XML_NO_OF_ATRIBUTES)
	{
		OOBSIManager::add_program_info( source_id, &info);
	}
	return i;	
}

void xml_parse( TiXmlNode* pParent )
{
	if ( !pParent ) return;

	TiXmlNode* pChild;
	int t = pParent->Type();

	if  ( t  == TiXmlNode::TINYXML_ELEMENT)
	{
		//printf( "Element [%s]", pParent->Value() );
		if ( 0 == strcmp(pParent->Value(), XML_PGM_ELEMENT_NAME))
			xml_parse_attributes(pParent->ToElement());
	}
	for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
	{
		xml_parse( pChild );
	}
}



OOBSIManager* OOBSIManager::oobSIManager = NULL;
OOBSIManager::OOBSIManager( const char* file)
{
	//std::cout << __FUNCTION__ << std::endl;
	TiXmlDocument doc(file);
	bool loadOkay = doc.LoadFile();
	if (loadOkay)
	{
		//printf("\n%s:\n", file);
		xml_parse( &doc );
	}
	else
	{
		printf("Failed to load file \"%s\"\n", file);
	}
}

/**
 * @fn OOBSIManager::getInstance()
 * @brief This function creates the instance of OOBSIManager if not already created and returns the pointer to it.
 * It is being used to get the program specific information.
 *
 * @return oobSIManager Returns pointer to the OOBSIManager instance.
 */
OOBSIManager* OOBSIManager::getInstance()
{
       //std::cout << __FUNCTION__ << std::endl;
	if (NULL == oobSIManager)
	{
		oobSIManager = new OOBSIManager("sidb.xml");
	}
	return oobSIManager;
}

/**
 * @fn OOBSIManager::GetProgramInfo ( uint32_t sourceID, rmf_ProgramInfo_t* p_info)
 * @brief This function gets the modulation, frequency and program number corresponding to the specified sourceID.
 *
 * @param[in] sourceID Unique ID given for the program.
 * @param[out] p_info Pointer to the program information list.
 *
 * @return Returns RMF_SUCCESS if an entry against the sourceID is present in the program info list else returns 5.
 */
rmf_Error  OOBSIManager::GetProgramInfo ( uint32_t sourceID, rmf_ProgramInfo_t* p_info)
{
	program_info_list_s* itr;
	for (itr = g_program_info_list; itr; itr = itr->next )
	{
		if ( itr->sourceID == sourceID )
			break;
	}
	if (itr)
	{
		p_info->carrier_frequency = itr->info.carrier_frequency*1000000;
		p_info->modulation_mode = itr->info.modulation_mode;
		p_info->symbol_rate = itr->info.symbol_rate;
		p_info->prog_num = itr->info.prog_num;
		return RMF_SUCCESS;
	}
	else
	{
		return 5;
	}
}

/**
 * @fn OOBSIManager::add_program_info ( uint32_t sourceID, rmf_ProgramInfo_t* p_info)
 * @brief This function adds a new node with the specified sourceID and its program information
 * to the list of program information.
 *
 * @param[in] sourceID Unique ID to identify the program.
 * @param[in] p_info Pointer to the program information corresponding to the specified sourceID.
 *
 * @return Returns RMF_SUCCESS after adding the sourceID and its program information into the list.
 */
rmf_Error  OOBSIManager::add_program_info ( uint32_t sourceID, rmf_ProgramInfo_t* p_info)
{
	program_info_list_s* node = new program_info_list_s();
	//std::cout << __FUNCTION__ << std::endl;
	node->next = g_program_info_list;
	node->sourceID = sourceID;
	node->info.carrier_frequency = p_info->carrier_frequency;
	node->info.modulation_mode = p_info->modulation_mode;
	node->info.prog_num = p_info->prog_num;
	node->info.symbol_rate = p_info->symbol_rate;
	g_program_info_list = node;
	return RMF_SUCCESS;
}

/**
 * @fn OOBSIManager::dump_program_info ( const char* path )
 * @brief This function is used to save the program information list into a document in the specified path.
 *
 * @param[in] path Indicates the path to save the file created.
 *
 * @return Returns RMF_SUCCESS if the document is created and saved successfully else returns -1.
 */
rmf_Error  OOBSIManager::dump_program_info ( const char* path )
{
	std::cout << __FUNCTION__ <<":"<<__LINE__<< std::endl;
	TiXmlDocument doc;  
	TiXmlDeclaration* decl = new TiXmlDeclaration( XML_DECLARATION );  
	doc.LinkEndChild( decl );  
 
	TiXmlElement * root = new TiXmlElement( XML_ELEMENT_1STLEVEL);  
	if (NULL == doc.LinkEndChild( root ) )
	{
		printf("%s:%d: LinkEndChild failed\n", __FUNCTION__, __LINE__);
		return -1;
	}

	TiXmlComment * comment = new TiXmlComment();
	comment->SetValue(" Test settings" );  
	root->LinkEndChild( comment );  
	program_info_list_s* itr;
	for (itr = g_program_info_list; itr; itr = itr->next )
	{
		TiXmlElement * program;
		program = new TiXmlElement( XML_PGM_ELEMENT_NAME);  
		if (NULL != root->LinkEndChild( program ) )
		{
			program->SetAttribute( XML_SOURCE_ID_KEY, itr->sourceID);
			program->SetAttribute( XML_FREQ_KEY, itr->info.carrier_frequency);
			program->SetAttribute( XML_MODULATION_KEY, itr->info.modulation_mode);
			program->SetAttribute( XML_PGM_NO_KEY, itr->info.prog_num);
		}
	}
	doc.SaveFile( path);  
	return RMF_SUCCESS;
}

/**
 * @fn OOBSIManager::get_source_id_vector(std::vector<uint32_t> &source_id_list)
 * @brief This function is used to add all the source ID's listed in g_program_info_list into
 * the given vector source_id_list.
 *
 * @param[in] source_id_list A vector to store the source ID's.
 *
 * @return None
 */
void OOBSIManager::get_source_id_vector(std::vector<uint32_t> &source_id_list)
{
	program_info_list_s* itr;
	for (itr = g_program_info_list; itr; itr = itr->next )
	{
		source_id_list.insert(source_id_list.begin(), itr->sourceID);
	}
}

