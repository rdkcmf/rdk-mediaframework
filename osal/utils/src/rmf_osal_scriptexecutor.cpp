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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>

//#include "vlMemCpy.h"
//#include "utilityMacros.h"
#include "rmf_osal_tcpsocketserver.h"
#include "rmf_osal_udpsocketserver.h"
#include "rmf_osal_scriptexecutor.h"

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#endif

#include "rdk_debug.h"
 
#define RMF_OSAL_SCRIPT_EXEC_LOG(x...) RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SYS", x);


typedef void    ( *RMF_OSAL_SCRIPT_EXEC_VOID_FUNC             )(void);

/*-----------------------------------------------------------------------------*/
/* Parser States */
/*-----------------------------------------------------------------------------*/
typedef enum _RMF_OSAL_SCRIPT_EXEC_STATE
{
    RMF_OSAL_SCRIPT_EXEC_STATE_START,
    RMF_OSAL_SCRIPT_EXEC_STATE_ASSIGN_OR_EXECUTE,
    RMF_OSAL_SCRIPT_EXEC_STATE_ASSIGN_GET_VALUE,
    RMF_OSAL_SCRIPT_EXEC_STATE_EXECUTE_GET_PARAMS,
    RMF_OSAL_SCRIPT_EXEC_STATE_END,
}RMF_OSAL_SCRIPT_EXEC_STATE;

/*-----------------------------------------------------------------------------*/
/* Local # defines */
/*-----------------------------------------------------------------------------*/
#define RMF_OSAL_SCRIPT_EXEC_TOKEN_MAX        0x100
#define RMF_OSAL_SCRIPT_EXEC_STATEMENT_MAX    0x100
#define RMF_OSAL_MAX_SHELL_PARAMS     0x10
#define RMF_OSAL_ISALPHA(c)  (isalpha(c) || ('_' == (c)))
#define RMF_OSAL_ISALNUM(c)  (isalnum(c) || ('_' == (c)))

typedef struct _RMF_OSAL_SCRIPT_EXECUTOR
{
    char * pszScript;
    FILE * pFile;
    int  sockfd;
    
    RMF_OSAL_SCRIPT_EXEC_SYMBOL_ENTRY *   paSymbols;
    int                 nSymbols;
    
    unsigned long *pSymbol;
    RMF_OSAL_SCRIPT_EXEC_STATE eState;
    int iParams;
    int  nVars;
    int aParams[RMF_OSAL_MAX_SHELL_PARAMS];
    char aParamTokens[RMF_OSAL_MAX_SHELL_PARAMS][RMF_OSAL_SCRIPT_EXEC_TOKEN_MAX];
    char szSymName[RMF_OSAL_SCRIPT_EXEC_TOKEN_MAX];
    RMF_OSAL_SCRIPT_EXEC_SYM_TYPE symType;
    int  nSign;
    
    char szToken[RMF_OSAL_SCRIPT_EXEC_TOKEN_MAX];
    char * pszCurLine;
    char * pszNextToken;
}RMF_OSAL_SCRIPT_EXECUTOR;

static int rmf_osal_GetSymbol(RMF_OSAL_SCRIPT_EXECUTOR * pExecutor, char * pszName, RMF_OSAL_SCRIPT_EXEC_SYM_TYPE * pSymType, unsigned long ** pVar, int * pnVars)
{
    int i = 0;
    
    for(i = 0; i < pExecutor->nSymbols; i++)
    {
        if(0 == strcmp(pszName, pExecutor->paSymbols[i].pszName))
        {
            *pSymType = pExecutor->paSymbols[i].eSymType;
            *pVar     = pExecutor->paSymbols[i].pVar;
            *pnVars   = pExecutor->paSymbols[i].nVars;
            return 1;
        }
    }
    
    return 0;
};

/*-----------------------------------------------------------------------------*/
#define RMF_OSAL_ADD_CHAR_TO_TOKEN()                                              \
    {   /* add char to token */                                             \
        pExecutor->szToken[iPos] = *pszLine;                                \
        /* increment pointers and indices */                                \
        iPos++;                                                             \
        pszLine++;                                                          \
        /* check for end-of-string */                                       \
        if('\0' == (unsigned char)(*pszLine))                               \
        {                                                                   \
            pExecutor->pszNextToken = pszLine;                              \
            /* RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_GetToken: Returning token '%s'.\n", szToken); */   \
            return pExecutor->szToken;                                      \
        }                                                                   \
        /* check for buffer overflow */                                     \
        if((unsigned int)iPos >= sizeof(pExecutor->szToken))                \
        {                                                                   \
            RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_GetToken: ERROR: Buffer overflow while reading token '%s'.\n", pszLine); \
            iPos = sizeof(pExecutor->szToken)-1;                            \
        }                                                                   \
    }

/*-----------------------------------------------------------------------------*/
static char * rmf_osal_GetToken(RMF_OSAL_SCRIPT_EXECUTOR * pExecutor)
{
    /* local declarations */
    char * pszLine = pExecutor->pszNextToken;
    int iPos = 0;

    /* zero the buffer */
    memset(pExecutor->szToken, 0, sizeof(pExecutor->szToken));

    /* skip white space */
    while(isspace(*pszLine))
    {
        if('\n' == *pszLine)
        {
            pExecutor->pszNextToken = pszLine;
            return NULL;
        }
        pszLine++;
    }

    /* check for end-of-line */
    if('\0' == *pszLine)
    {
        pExecutor->pszNextToken = pszLine;
        return NULL;
    }
    /* check for comments */
    else if('#' == *pszLine)
    {
        /* treat comments as you would treat an end-of-line */
        pExecutor->pszNextToken = pszLine;
        return NULL;
    }
    /* check for alpha-numeric */
    else if(RMF_OSAL_ISALNUM(*pszLine))
    {
        /* while alpha-numeric */
        while(RMF_OSAL_ISALNUM(*pszLine))
        {
            RMF_OSAL_ADD_CHAR_TO_TOKEN();
        }
    }
    /* check for strings */
    else if('"' == *pszLine)
    {
        do
        {
            RMF_OSAL_ADD_CHAR_TO_TOKEN();
        }
        while('"' != *pszLine);
        /* till the end-of-string */

        /* add the trailing quote also */
        RMF_OSAL_ADD_CHAR_TO_TOKEN();
    }
    /* check for char-literals */
    else if('\'' == *pszLine)
    {
        do
        {
            RMF_OSAL_ADD_CHAR_TO_TOKEN();
        }
        while('\'' != *pszLine);
        /* till the end-of-literal */

        /* add the trailing quote also */
        RMF_OSAL_ADD_CHAR_TO_TOKEN();
    }
    else if(ispunct(*pszLine))
    {
        /* add the symbol */
        RMF_OSAL_ADD_CHAR_TO_TOKEN();
    }
    else
    {
        RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_GetToken: ERROR: Found unexpected token '%s'.\n", pszLine);
    }

    /* re-position for next token */
    pExecutor->pszNextToken = pszLine;
    /* RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_GetToken: Returning token '%s'.\n", szToken); */
    return pExecutor->szToken;
}

/*-----------------------------------------------------------------------------*/
static char * rmf_osal_GetNextToken(RMF_OSAL_SCRIPT_EXECUTOR * pExecutor, char * pszLine, int bNewLine)
{
    /* static declarations */

    /* check for new line */
    if(bNewLine)
    {
        /* re-initialize pointers for new line */
        pExecutor->pszCurLine   = pszLine;
        pExecutor->pszNextToken = pszLine;
    }

    /* get and return next token */
    return rmf_osal_GetToken(pExecutor);
}

/*-----------------------------------------------------------------------------*/
/* Function Prototypes */
/*-----------------------------------------------------------------------------*/
typedef int (*RMF_OSAL_SHELL_FUNC_0 )();
typedef int (*RMF_OSAL_SHELL_FUNC_1 )(int);
typedef int (*RMF_OSAL_SHELL_FUNC_2 )(int, int);
typedef int (*RMF_OSAL_SHELL_FUNC_3 )(int, int, int);
typedef int (*RMF_OSAL_SHELL_FUNC_4 )(int, int, int, int);
typedef int (*RMF_OSAL_SHELL_FUNC_5 )(int, int, int, int, int);
typedef int (*RMF_OSAL_SHELL_FUNC_6 )(int, int, int, int, int, int);
typedef int (*RMF_OSAL_SHELL_FUNC_7 )(int, int, int, int, int, int, int);
typedef int (*RMF_OSAL_SHELL_FUNC_8 )(int, int, int, int, int, int, int, int);
typedef int (*RMF_OSAL_SHELL_FUNC_9 )(int, int, int, int, int, int, int, int, int);
typedef int (*RMF_OSAL_SHELL_FUNC_10)(int, int, int, int, int, int, int, int, int, int);

/*-----------------------------------------------------------------------------*/
static int rmf_osal_CallFunction(RMF_OSAL_SCRIPT_EXECUTOR * pExecutor, int aParams[],
                            char   aParamTokens[RMF_OSAL_MAX_SHELL_PARAMS][RMF_OSAL_SCRIPT_EXEC_TOKEN_MAX])
{
    /* local declarations */
    char * pszFuncName      = pExecutor->szSymName;
    unsigned long * pFunc   = (unsigned long *)(pExecutor->pSymbol);
    int nExpectedParams     = pExecutor->nVars;
    int nSuppliedParams     = pExecutor->iParams;
    
    int i    = 0;
    int nRet = -1;

    /* check for NULL function */
    if(NULL == pFunc)
    {
        RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_CallFunction: ERROR: Skipped call to missing function '%s'.\n\n", pszFuncName);
        return -1;
    }

    /*RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_CallFunction: INFO: Calling function %s(", pszFuncName);

    for(i = 0; i < nSuppliedParams; i++)
    {
        if(i > 0)
        {
            RMF_OSAL_SCRIPT_EXEC_LOG(", ");
        }

        RMF_OSAL_SCRIPT_EXEC_LOG("0x%x [%s]", aParams[i], aParamTokens[i]);
    }

    RMF_OSAL_SCRIPT_EXEC_LOG(")\n");*/
    
    if(RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_FUNC != pExecutor->symType)
    {
        RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_CallFunction: ERROR: %s() is not registered as a function.\n\n", pszFuncName);
        return -1;
    }

    if(nExpectedParams != nSuppliedParams)
    {
        RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_CallFunction: WARNING: %s() expects %d parameters, but provided %d parameters\n\n", pszFuncName, nExpectedParams, nSuppliedParams);
    }

    /* call the function */
    switch(nExpectedParams)
    {
        case  0: nRet = ((RMF_OSAL_SHELL_FUNC_0 )pFunc)();
            break;
        case  1: nRet = ((RMF_OSAL_SHELL_FUNC_1 )pFunc)(aParams[0]);
            break;
        case  2: nRet = ((RMF_OSAL_SHELL_FUNC_2 )pFunc)(aParams[0], aParams[1]);
            break;
        case  3: nRet = ((RMF_OSAL_SHELL_FUNC_3 )pFunc)(aParams[0], aParams[1], aParams[2]);
            break;
        case  4: nRet = ((RMF_OSAL_SHELL_FUNC_4 )pFunc)(aParams[0], aParams[1], aParams[2], aParams[3]);
            break;
        case  5: nRet = ((RMF_OSAL_SHELL_FUNC_5 )pFunc)(aParams[0], aParams[1], aParams[2], aParams[3], aParams[4]);
            break;
        case  6: nRet = ((RMF_OSAL_SHELL_FUNC_6 )pFunc)(aParams[0], aParams[1], aParams[2], aParams[3], aParams[4], aParams[5]);
            break;
        case  7: nRet = ((RMF_OSAL_SHELL_FUNC_7 )pFunc)(aParams[0], aParams[1], aParams[2], aParams[3], aParams[4], aParams[5], aParams[6]);
            break;
        case  8: nRet = ((RMF_OSAL_SHELL_FUNC_8 )pFunc)(aParams[0], aParams[1], aParams[2], aParams[3], aParams[4], aParams[5], aParams[6], aParams[7]);
            break;
        case  9: nRet = ((RMF_OSAL_SHELL_FUNC_9 )pFunc)(aParams[0], aParams[1], aParams[2], aParams[3], aParams[4], aParams[5], aParams[6], aParams[7], aParams[8]);
            break;
        case 10: nRet = ((RMF_OSAL_SHELL_FUNC_10)pFunc)(aParams[0], aParams[1], aParams[2], aParams[3], aParams[4], aParams[5], aParams[6], aParams[7], aParams[8], aParams[9]);
            break;

        default:
        {
            RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_CallFunction: ERROR: Too many parameters for function '%s', nParams = %d.\n", pszFuncName, nExpectedParams);
        }
        break;
    }

    //RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_CallFunction: INFO: Function '%s()' returned %d (0x%x)\n\n", pszFuncName, nRet, nRet);

    /* return the value returned by the function */
    return nRet;
}

/*-----------------------------------------------------------------------------*/
static int rmf_osal_GetValue(RMF_OSAL_SCRIPT_EXECUTOR * pExecutor, char * pszToken, int * pnSign)
{
    /* local declarations */
    int nSign = *pnSign;
    *pnSign = 1;

    /* check for NULL and end-of-string */
    if((NULL == pszToken) || ('\0' == pszToken[0]))
    {
        return 0;
    }

    /* numbers starting with '0' */
    if('0' == *pszToken)
    {
        /* Hexadecimal numbers */
        if(('x' == pszToken[1]) && ('\0' != pszToken[2]))
        {
            return(nSign * strtoul(pszToken+2, NULL, 16));
        }
        /* Octal numbers */
        else if('\0' != pszToken[1])
        {
            return(nSign * strtoul(pszToken+1, NULL, 8));
        }
        /* Regular decimal '0' */
        else
        {
            return(nSign * strtoul(pszToken, NULL, 10));
        }
    }
    /* Regular decimal numbers */
    else if(isdigit(*pszToken))
    {
        return(nSign * strtoul(pszToken, NULL, 10));
    }
    /* char literals */
    else if(('\'' == *pszToken) && ('\0' != pszToken[1]))
    {
        return pszToken[1];
    }
    /* strings */
    else if(('"' == *pszToken) && ('\0' != pszToken[1]))
    {
        int nStrLen = strlen(pszToken);
        char * pStr = (char*)malloc(nStrLen+1);
        memset(pStr, 0, nStrLen);
        if(nStrLen >= 2)
        {
            strncpy(pStr, pszToken+1, nStrLen+1);
            pStr[nStrLen-2] = '\0';
        }
        return((intptr_t)pStr);
    }
    /* variable identifiers */
    else if(RMF_OSAL_ISALPHA(*pszToken))
    {
        unsigned long * pSymbol   = NULL;
        int nVars = 0;
        RMF_OSAL_SCRIPT_EXEC_SYM_TYPE symType = RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_NONE;
        /* try locating the variable */
        if(rmf_osal_GetSymbol(pExecutor, pszToken, &symType, &pSymbol, &nVars))
        {
            /* return the contents of the variable */
            /* non-integer vaiables might cause data-alignment exception here */
            return *pSymbol;
        }
        else
        {
            RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_GetValue: ERROR: Could not find symbol '%s'.\n", pszToken);
        }
    }

    RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_GetValue: ERROR: Could not process token '%s'.\n", pszToken);
    return 0;
}

/*-----------------------------------------------------------------------------*/
static int rmf_osal_AssignValue(RMF_OSAL_SCRIPT_EXECUTOR * pExecutor, char * pszToken, int nValue)
{
    if(RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_LVAL != pExecutor->symType)
    {
        RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_AssignValue: ERROR: Cannot assign %s = %d (0x%x, %s) as '%s' is not registered as an lvalue.\n\n", pExecutor->szSymName, nValue, nValue, pszToken, pExecutor->szSymName);
        return -1;
    }
    else
    {
        //RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_AssignValue: INFO: Assigning %s = %d (0x%x, %s).\n\n", pExecutor->szSymName, nValue, nValue, pszToken);
        /* assign the variable with new value */
        /* non-integer vaiables might cause data-alignment exception here */
        switch(pExecutor->nVars)
        {
            case 1:
            {
                *((char*)(pExecutor->pSymbol)) = (char)nValue;
            }
            break;
                                    
            case 2:
            {
                *((short*)(pExecutor->pSymbol)) = (short)nValue;
            }
            break;
                                                            
            case 4:
            {
                *((long*)(pExecutor->pSymbol)) = (long)nValue;
            }
            break;
                                                                                    
            case 8:
            {
                *((long long*)(pExecutor->pSymbol)) = (long long)nValue;
            }
            break;
                                    
            default:
            {
                *((int*)(pExecutor->pSymbol)) = (int)nValue;
            }
            break;
        }
    }
    
    return 0;
}
/*-----------------------------------------------------------------------------*/
static RMF_OSAL_SCRIPT_EXEC_STATE rmf_osal_ParseToken(RMF_OSAL_SCRIPT_EXECUTOR * pExecutor, char * pszToken, int bNewLine)
{
    RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_ParseCommand: INFO: parsing '%s'.\n", pszToken);

    /* check for new line */
    if(bNewLine)
    {
        /* process and reign-in any dangling states */
        if(RMF_OSAL_SCRIPT_EXEC_STATE_EXECUTE_GET_PARAMS == pExecutor->eState)
        {
            RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_ParseCommand: WARNING: Encountered new line while processing function '%s'. Calling the dangling function.\n", pExecutor->szSymName);
            /* call the dangling function */
            rmf_osal_CallFunction(pExecutor,
                           pExecutor->aParams, pExecutor->aParamTokens);
        }
        else if(RMF_OSAL_SCRIPT_EXEC_STATE_ASSIGN_GET_VALUE == pExecutor->eState)
        {
            RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_ParseCommand: WARNING: Encountered new line while processing assignment for '%s'.\n", pExecutor->szSymName);
        }

        /* re-initialize the state-machine */
        pExecutor->eState  = RMF_OSAL_SCRIPT_EXEC_STATE_START;
        pExecutor->pSymbol = NULL;
        pExecutor->iParams = 0;
        pExecutor->nSign   = 1;
    }

    /* check for NULL */
    if(NULL == pszToken)
    {
        return pExecutor->eState;
    }
    /* check for end-of-file */
    else if(0 == strcmp("EOF", pszToken))
    {
        pExecutor->eState = RMF_OSAL_SCRIPT_EXEC_STATE_END;
        return pExecutor->eState;
    }
    /* check for end-of-string */
    else if('\0' == *pszToken)
    {
        return pExecutor->eState;
    }
    /* check for comments */
    else if('#' == *pszToken)
    {
        return pExecutor->eState;
    }
    else
    {
        /* handle the state */
        switch(pExecutor->eState)
        {
            case RMF_OSAL_SCRIPT_EXEC_STATE_START:
            {
                if(RMF_OSAL_ISALPHA(*pszToken))
                {
                    /* make a copy of the symbol */
                    strncpy(pExecutor->szSymName, pszToken, sizeof(pExecutor->szSymName));
                    pExecutor->szSymName[RMF_OSAL_SCRIPT_EXEC_TOKEN_MAX-1] = '\0';
                    /* locate the symbol */
                    if(!rmf_osal_GetSymbol(pExecutor, pszToken, &(pExecutor->symType), &(pExecutor->pSymbol), &(pExecutor->nVars)))
                    {
                        RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_ParseCommand: ERROR: Could not find symbol '%s'.\n", pszToken);
                        pExecutor->pSymbol = NULL;
                    }

                    /* transition to next state */
                    pExecutor->eState = RMF_OSAL_SCRIPT_EXEC_STATE_ASSIGN_OR_EXECUTE;
                }
            }
            break;

            case RMF_OSAL_SCRIPT_EXEC_STATE_ASSIGN_OR_EXECUTE:
            {
                /* check for assignment command */
                if('=' == *pszToken)
                {
                    /* transition to next state */
                    pExecutor->eState  = RMF_OSAL_SCRIPT_EXEC_STATE_ASSIGN_GET_VALUE;
                }
                else
                {
                    /* transition to next state */
                    pExecutor->eState  = RMF_OSAL_SCRIPT_EXEC_STATE_EXECUTE_GET_PARAMS;
                    pExecutor->iParams = 0;
                }
            }
            break;

            case RMF_OSAL_SCRIPT_EXEC_STATE_ASSIGN_GET_VALUE:
            {
                /* register any possible value negation with unary '-' */
                if('-' == *pszToken)
                {
                    pExecutor->nSign = -1;
                    break;
                }
                else
                {
                    /* if symbol has been located */
                    if(NULL != pExecutor->pSymbol)
                    {
                        /* get the value in token */
                        int nValue = rmf_osal_GetValue(pExecutor, pszToken, &(pExecutor->nSign));

                        rmf_osal_AssignValue(pExecutor, pszToken, nValue);
                    }
                    else
                    {
                        RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_ParseCommand: ERROR: Skipped Assignment of missing '%s'.\n\n", pExecutor->szSymName);
                    }

                    /* re-initialize the state-machine */
                    pExecutor->eState  = RMF_OSAL_SCRIPT_EXEC_STATE_START;
                    pExecutor->pSymbol = NULL;
                    pExecutor->iParams = 0;
                    pExecutor->nSign   = 1;
                }
            }
            break;

            case RMF_OSAL_SCRIPT_EXEC_STATE_EXECUTE_GET_PARAMS:
            {
                /* register any possible value negation with unary '-' */
                if('-' == *pszToken)
                {
                    pExecutor->nSign = -1;
                    break;
                }
                /* start of function params */
                else if('(' == *pszToken)
                {
                    break;
                }
                /* function param separators */
                else if(',' == *pszToken)
                {
                    break;
                }
                /* end of function params */
                else if(')' == *pszToken)
                {
                    /* call the function */
                    rmf_osal_CallFunction(pExecutor,
                                   pExecutor->aParams, pExecutor->aParamTokens);
                    /* re-initialize the state-machine */
                    pExecutor->eState  = RMF_OSAL_SCRIPT_EXEC_STATE_START;
                    pExecutor->pSymbol = NULL;
                    pExecutor->iParams = 0;
                    pExecutor->nSign   = 1;
                }
                else
                {
                    /* make a copy of the token */
                    strncpy(pExecutor->aParamTokens[pExecutor->iParams], pszToken, sizeof(pExecutor->aParamTokens[pExecutor->iParams]));
                    pExecutor->aParamTokens[pExecutor->iParams][RMF_OSAL_SCRIPT_EXEC_TOKEN_MAX-1] = '\0';
                    /* register the value for this parameter */
                    pExecutor->aParams[pExecutor->iParams] = rmf_osal_GetValue(pExecutor, pszToken, &(pExecutor->nSign));
                    /* increment param pointer */
                    pExecutor->iParams++;
                    /* check for parameter overflow */
                    if(pExecutor->iParams >= RMF_OSAL_MAX_SHELL_PARAMS)
                    {
                        RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_ParseCommand: ERROR: Too many function parameters.\n");
                        /* bound the parameter pointer */
                        pExecutor->iParams = RMF_OSAL_MAX_SHELL_PARAMS-1;
                    }
                }
            }
            break;

            default:
            {
                //RMF_OSAL_SCRIPT_EXEC_LOG("rmf_osal_ParseCommand: INFO: Entered default state.\n");
                /* transition to start state */
                pExecutor->eState = RMF_OSAL_SCRIPT_EXEC_STATE_START;
            }
            break;
        }
    }

    /* return current state */
    return pExecutor->eState;
}

/********************************************************************************
 * rmf_osal_ExecuteScript - script executer
 *
 */

/*-----------------------------------------------------------------------------*/
static void rmf_osal_ExecuteScript(RMF_OSAL_SCRIPT_EXECUTOR * pExecutor)
{
    /* local declarations */
    char szBuf[RMF_OSAL_SCRIPT_EXEC_STATEMENT_MAX];
    char * pszRead  = NULL;
    char * pszToken = NULL;
    int bNewLine = 0;

    RMF_OSAL_SCRIPT_EXEC_LOG("\n rmf_osal_ExecuteScript: INFO: Executing sript: '%s'\n", pExecutor->pszScript);

    /* check for missing file */
    if(NULL != pExecutor->pFile)
    {
        do
        {
            /* zero the buffer */
            memset(szBuf, 0, sizeof(szBuf));

            /* read a line from file */
            pszRead  = fgets(szBuf, sizeof(szBuf), pExecutor->pFile);

            /* if not end-of-file */
            if(NULL != pszRead)
            {
                /* announce start of new line */
                bNewLine = 1;
                /* while tokens are available for parsing */
                while(NULL != (pszToken = rmf_osal_GetNextToken(pExecutor, pszRead, bNewLine)))
                {
        RMF_OSAL_SCRIPT_EXEC_LOG("\n rmf_osal_ExecuteScript: pszToken: '%s'\n\n", pszToken);
                    /* parse the token */
                    RMF_OSAL_SCRIPT_EXEC_STATE eState = rmf_osal_ParseToken(pExecutor, pszToken, bNewLine);
                    /* de-announce new line */
                    bNewLine = 0;
                    
                    if(RMF_OSAL_SCRIPT_EXEC_STATE_END == eState)
                    {
                        return;
                    }
                }
            }
        }
        while(NULL != pszRead);
        /* till end-of-file */

        RMF_OSAL_SCRIPT_EXEC_LOG("\n rmf_osal_ExecuteScript: INFO: Completed execution of script '%s'\n\n", pExecutor->pszScript);
    }
    else
    {
        RMF_OSAL_SCRIPT_EXEC_LOG("\n rmf_osal_ExecuteScript: ERROR: Could not find script: '%s'\n\n", pExecutor->pszScript);
    }
}

void rmf_osal_ExecuteFileScript(char * pszScript, void * _pTable)
{
    RMF_OSAL_SCRIPT_EXEC_SYM_TBL * pTable = (RMF_OSAL_SCRIPT_EXEC_SYM_TBL*)_pTable;
    
    if(NULL != pszScript)
    {
        RMF_OSAL_SCRIPT_EXECUTOR scriptExecutor;
        //VL_ZERO_STRUCT(scriptExecutor);
        memset(&scriptExecutor, 0x0, sizeof(RMF_OSAL_SCRIPT_EXECUTOR));
        scriptExecutor.paSymbols = pTable->paSymbols;
        scriptExecutor.nSymbols = pTable->nSymbols;
        scriptExecutor.pszScript = pszScript;
        /* open the file */
        scriptExecutor.pFile = fopen (scriptExecutor.pszScript, "r");
        if(NULL != scriptExecutor.pFile)
        {
            rmf_osal_ExecuteScript(&scriptExecutor);
            fclose(scriptExecutor.pFile);
        }
    }
    else
    {
        RMF_OSAL_SCRIPT_EXEC_LOG("\n rmf_osal_ExecuteFileScript: ERROR: pszScript is NULL\n\n");
    }
}

void rmf_osal_ExecuteSocketScript(int sockfd, void * _pTable)
{
    RMF_OSAL_SCRIPT_EXEC_SYM_TBL * pTable = (RMF_OSAL_SCRIPT_EXEC_SYM_TBL*)_pTable;
    
    if((0 != sockfd) && (-1 != sockfd))
    {
        RMF_OSAL_SCRIPT_EXECUTOR scriptExecutor;
        //VL_ZERO_STRUCT(scriptExecutor);
        memset(&scriptExecutor, 0x0, sizeof(RMF_OSAL_SCRIPT_EXECUTOR));
        scriptExecutor.paSymbols = pTable->paSymbols;
        scriptExecutor.nSymbols = pTable->nSymbols;
        scriptExecutor.pszScript = (char*)("socket");
        /* open the file */
        scriptExecutor.pFile = fdopen (sockfd, "rw");
        scriptExecutor.sockfd  = sockfd;
        if(NULL != scriptExecutor.pFile)
        {
            rmf_osal_ExecuteScript(&scriptExecutor);
            fclose(scriptExecutor.pFile);
        }
    }
    else
    {
        RMF_OSAL_SCRIPT_EXEC_LOG("\n rmf_osal_ExecuteSocketScript: ERROR: sockfd is %d\n\n", sockfd);
    }
}

int rmf_osal_CreateTcpScriptServer(int nPort, RMF_OSAL_SCRIPT_EXEC_SYM_TBL * pTable)
{
    RMF_OSAL_SCRIPT_EXEC_SYM_TBL * pTableCopy =
            (RMF_OSAL_SCRIPT_EXEC_SYM_TBL*)malloc(sizeof(RMF_OSAL_SCRIPT_EXEC_SYM_TBL));
    *pTableCopy = *pTable;
    return rmf_osal_CreateTcpSocketServer(nPort, rmf_osal_ExecuteSocketScript, pTableCopy);
}


int rmf_osal_CreateUdpScriptServer(int nPort, RMF_OSAL_SCRIPT_EXEC_SYM_TBL * pTable)
{
    RMF_OSAL_SCRIPT_EXEC_SYM_TBL * pTableCopy =
            (RMF_OSAL_SCRIPT_EXEC_SYM_TBL*)malloc(sizeof(RMF_OSAL_SCRIPT_EXEC_SYM_TBL));
    *pTableCopy = *pTable;
    return rmf_osal_CreateUdpSocketServer(nPort, rmf_osal_ExecuteSocketScript, pTableCopy);
}
