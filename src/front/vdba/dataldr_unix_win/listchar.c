/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : listchar.c : Implementation file
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : list of string / tool for manipulation token string
**
** History:
**
** xx-Jan-2004 (uk$so01)
**    Created.
** 15-Dec-2004 (toumi01)
**    Port to Linux.
**    06-May-2005 (hanje04)
**	malloc.h is sys/malloc.h on OSX
**	08-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
*/

#include "stdafx.h"
#include "listchar.h"
# ifdef OSX
# include <sys/malloc.h>
# else
#include <malloc.h>
# endif
#if defined (UNIX)
#include <stdarg.h>
#endif

char stack_top(char* stack)
{
	if (stack)
		return stack[0];
	return '\0';
}

void stack_PushState(char* stack, char state)
{
	int nLen = 0;
	if (!stack)
		return;
	nLen = STlength (stack);
	if (nLen > 0)
	{
		stack[nLen-1] = state;
		stack[nLen] = '\0';
	}
	else
	{
		stack[0] = state;
		stack[1] = '\0';
	}
}

void stack_PopState(char* stack)
{
	int nLen = 0;
	if (!stack)
		return;
	nLen = STlength (stack);
	if (nLen > 0)
	{
		stack[nLen-1] = '\0';
	}
	else
	{
		stack[0] = '\0';
	}
}



/*
** Get the next character:
** It does not modify the parameter 'lpcText'
*/
char PeekNextChar (const char* lpcText)
{
	char* pch = (char*)lpcText;
	if (!pch)
		return '\0';
	CMnext(pch);
	if (!pch)
		return '\0';
	return *pch;
}


int ExtractValue (char* pToken, int nKeyLen, char* szValue)
{
	int nEqual = 0;
	char* pItem = &pToken[nKeyLen];
	szValue[0] = '\0';
	if (pItem)
	{
		if (pItem[0] == '=')
		{
			nEqual = 1;
			CMnext(pItem);
			STcopy (pItem, szValue);
		}
	}
	return nEqual;
}


char* TrimLeft(char* strText)
{
	char* strNew;
	int i, nLen = STlength(strText);
	strNew = strText;
	for (i=0; i<nLen; i++)
	{
		if (strText[i] == ' ' || strText[i] == '\t')
			strNew = &strText[i+1];
		else
			return strNew;
	}
	return strNew;
}

char* TrimRight(char* strText)
{
	int nLen = STlength(strText);
	while (nLen>0)
	{
		nLen--;
		if (strText[nLen] == ' ' || strText[nLen] =='\t')
			strText[nLen] = '\0';
		else
			return strText;
	}
	return strText;
}


/*
** ConcateStrings:
**
* The function that concates strings (ioBuffer, aString, ..) 
** 
** Parameter: 
**     1) ioBuffer: In Out Buffer 
**     2) aString : String to be concated.
**     3) ...     : Ellipse for other strings...with NULL terminated (char*)0 
** Return:
**     return  1 if Successful.
**     return  0 otherwise.
*/
int
ConcateStrings (char** ioBuffer, char* aString, ...)
{
	int     size;
	va_list Marker;
	char* strArg;

	if (!aString)
		return 0;

	if (!(*ioBuffer))
	{
		*ioBuffer = (char*)malloc (STlength (aString) +1);
		*ioBuffer[0] = '\0';
	}
	else
	{
		size      = STlength  ((char*)(*ioBuffer)) + STlength (aString) +1;
		*ioBuffer = (char*)realloc ((char*)(*ioBuffer), size);
	}

	if (!*ioBuffer)
		return 0;
	else
		STcat (*ioBuffer, aString);

	va_start (Marker, aString);
	while (strArg = va_arg (Marker, char*))
	{
		size      = STlength  (*ioBuffer) + STlength (strArg) +1;
		*ioBuffer = (char*)realloc(*ioBuffer, size);
		if (!*ioBuffer)
			return 0;
		else
			STcat (*ioBuffer, strArg);
	}
	va_end (Marker);
	return 1;
}

int ListChar_Count (LISTCHAR* oldList)
{
	int nCount = 0;
	LISTCHAR* ls = oldList;
	while (ls)
	{
		nCount++;
		ls = ls->next;
	}
	return nCount;
}


LISTCHAR* ListChar_Add  (LISTCHAR* oldList, char* szString)
{
	char* lpszText;
	LISTCHAR* lpStruct;
	LISTCHAR* ls;

	if (!szString)
		return oldList;
	lpszText = (char *) malloc (STlength (szString) +1);
	lpStruct = (LISTCHAR*)malloc (sizeof (LISTCHAR));
	if (!(lpszText && lpStruct))
	{
		return oldList;
	}

	STcopy (szString, lpszText);
	lpStruct->next      = NULL;
	lpStruct->prev      = NULL;
	lpStruct->pItem = lpszText;
	if (!oldList)
	{
		return lpStruct;
	}

	ls = oldList;
	while (ls)
	{
		if (!ls->next)
		{
			ls->next = lpStruct; 
			lpStruct->prev = ls;
			break;
		}
		ls = ls->next;
	}
	return oldList;
}

LISTCHAR* ListChar_Done (LISTCHAR* oldList)
{
	LISTCHAR* ls;
	LISTCHAR* lpTemp;
	ls = oldList;
	while (ls)
	{
		lpTemp = ls;
		ls = ls->next;
		free ((void*)(lpTemp->pItem));
		free ((void*)lpTemp);
	}
	return NULL;
}



LISTCHAR* AddToken(LISTCHAR* pListToken, char* strToken)
{
	char* strNewToken = TrimLeft(strToken);
	TrimRight(strNewToken);
	if (strNewToken && strNewToken[0])
		return ListChar_Add(pListToken, strNewToken);
	else
		return pListToken;
}

/*
** Tokens are separated by a space or quote:
** Ex1: "ABC=WXX /BN""X"/CL --> 2 token (inside the quote, the "" is equivalent to \")
**      1) "ABC=WXX /BN""X"
**      2) /CL
** Ex2: MAKE IT -->
**      1) MAKE
**      2) IT
*/
LISTCHAR* MakeListTokens(char* zsText, int* pError)
{
	char buff[8];
	char strToken[512];
	char stack[256];
	LISTCHAR* pListToken = NULL;
	char tchszCur = '0';
	char* pch = zsText;

	strToken[0] = '\0';
	memset (stack, '\0', sizeof (stack));
	while (*pch != '\0')
	{
		tchszCur = *pch;

		if (stack[0]) /* We're inside the quote (single or double) */
		{
			char tchTop = stack_top(stack);
			if (tchTop == tchszCur) /* The current character is a quote (single or double) */
			{
				if (PeekNextChar(pch) != tchTop) /* The next char is not a quote. (Here we found the end quote) */
				{
					stack_PopState(stack);
					STprintf(buff, "%c", tchszCur);
					STcat (strToken, buff);
				}
				else
				{
					/*
					** We consider the current quote the escaped of the next quote (consecutive quotes):
					*/
					STprintf(buff, "%c", tchszCur);
					STcat (strToken, buff);
					STprintf(buff, "%c", PeekNextChar(pch));
					STcat (strToken, buff);
					CMnext(pch);
				}
			}
			else
			{
				STprintf(buff, "%c", tchszCur);
				STcat (strToken, buff);
			}

			CMnext(pch);
			continue;
		} 

		/*
		** We found a space (token separator):
		*/
		if (tchszCur == ' ' || tchszCur == '='/* || tchszCur == cSep(!strTokenSeparator.IsEmpty() && strTokenSeparator.Find(tchszCur) != -1)*/)
		{
			if (strToken[0])
			{
				pListToken = AddToken(pListToken, strToken);
				strToken[0] = '\0';
			}
			if (tchszCur == '=')
			{
				pListToken = AddToken(pListToken, "=");
			}
		}
		else
		{
			if (tchszCur == '\"' || tchszCur == '\'') /* Found the begin quote.*/
			{
				/*
				** Note: the begin quote cannote be an escape of quote because 
				** the quote can be escaped only if it is inside the quote:
				*/
				stack_PushState (stack, tchszCur);
			}
			STprintf(buff, "%c", tchszCur);
			STcat (strToken, buff);
		}

		CMnext(pch);
		if (*pch == '\0')
		{
			if (strToken[0])
			{
				pListToken = AddToken(pListToken, strToken);
				strToken[0] = '\0';
			}
		}
	}
	if (strToken[0])
	{
		pListToken = AddToken(pListToken, strToken);
		strToken[0] = '\0';
	}

	if (stack[0])
	{
		pListToken = ListChar_Done(pListToken);
		if (pError)
			*pError = 1;
	}

	if (pError)
		*pError = 0;
	return pListToken;
}




