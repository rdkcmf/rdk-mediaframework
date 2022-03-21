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


/*-----------------------------------------------------------------*/
#ifndef __RMF_OSAL_SCRIPT_EXECUTOR_H__
#define __RMF_OSAL_SCRIPT_EXECUTOR_H__

/*-----------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------*/
typedef enum _RMF_OSAL_SCRIPT_EXEC_SYM_TYPE
{
    RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_NONE        = 0,
    RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_LVAL        = 1,
    RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_FUNC        = 2,

    RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_INVALID     = 0x1FFFFFFF
}RMF_OSAL_SCRIPT_EXEC_SYM_TYPE;

/*-----------------------------------------------------------------*/
typedef struct _RMF_OSAL_SCRIPT_EXEC_SYMBOL_ENTRY
{
    RMF_OSAL_SCRIPT_EXEC_SYM_TYPE     eSymType;
    char          * pszName;
    unsigned long * pVar;
    int             nVars;
}RMF_OSAL_SCRIPT_EXEC_SYMBOL_ENTRY;

/*-----------------------------------------------------------------*/
#define RMF_OSAL_SCRIPT_EXEC_MAKE_LVAL_SYMBOL_ENTRY(var)              {RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_LVAL, (char*)#var , (unsigned long*)(&(var)), sizeof(var)  }
#define RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(func, params)     {RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_FUNC, (char*)#func, (unsigned long*)(func  ), (params)     }

/*-----------------------------------------------------------------*/
typedef struct _RMF_OSAL_SCRIPT_EXEC_SYM_TBL
{
    int                 nSymbols;
    RMF_OSAL_SCRIPT_EXEC_SYMBOL_ENTRY *   paSymbols;
}RMF_OSAL_SCRIPT_EXEC_SYM_TBL;

/*-----------------------------------------------------------------*/
#define RMF_OSAL_SCRIPT_EXEC_INST_SYMBOL_TABLE(table, symbols)    \
static RMF_OSAL_SCRIPT_EXEC_SYM_TBL table = {                     \
    sizeof(symbols)/sizeof((symbols)[0]),                   \
           (symbols)                                        \
}

/*-----------------------------------------------------------------*/
void rmf_osal_ExecuteFileScript(char * pszScript, void * _pTable);
void rmf_osal_ExecuteSocketScript(int sockfd, void * _pTable);
int rmf_osal_CreateTcpScriptServer(int nPort, RMF_OSAL_SCRIPT_EXEC_SYM_TBL * pTable);
int rmf_osal_CreateUdpScriptServer(int nPort, RMF_OSAL_SCRIPT_EXEC_SYM_TBL * pTable);

/*-----------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

/*-----------------------------------------------------------------*/
#endif // __RMF_OSAL_SCRIPT_EXECUTOR_H__
/*-----------------------------------------------------------------*/
