/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : parse.c : Implementation file
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the command line & control file
**
** History:
**
** xx-Dec-2003 (uk$so01)
**    Created.
** 03-Mar-2004 (uk$so01)
**    BUG #111895 / Issue 13265678.
**    In data loader, if user specifies the control file without .ctl, 
**    then the program reports an error saying "Failed to create log file."
** 25-Mar-2004 (uk$so01)
**    BUG #112023 / ISSUE 13325452
**    The Readsize for dataldr command does not seem to be working as it is 
**    accepting any value.
**    CAUSE:
**         The actual readsize has a default value 65535. If user passes a 
**         value <= 0 or exceeds the maximum value of long, then dataldr 
**         will pick up the default value to continue working.
** 13-May-2004 (uk$so01)
**    BUG #112305, Replace the use of STindex by STrindex and x_strstr
** 11-Jun-2004 (uk$so01)
**    SIR #111688, additional change to support multiple INFILEs
** 29-Jun-2004 (uk$so01)
**    SIR #111688, additional change to support -u<username> option
**    at the command line.
** 13-Jul-2004 (noifr01)
**    SIR #111688, added support of the "QUOTED" keyword 
**    (after "TERMINATED BY <fieldend> )
** 19-Jul-2004 (noifr01)
**    SIR #111688, added support of PRESERVE BLANKS syntax, for keeping
**    trailing spaces at the end of columns
** 05-Aug-2004 (noifr01)
**    SIR #111688, added support of NULLSREPLACEDWITH syntax, for supporting
**    import of NULL characters through character substitution (reported in 
**    the log file)
** 05-Aug-2004 (noifr01)
**    SIR #111688, changed NULLSREPLACEDWITH keyword with NULSREPLACEDWITH
** 12-May-2006 (thaju02)
**    In ParseColumnDescription(), LNOBFILE designates long nvarchar col.
** 29-jan-2008 (huazh01)
**    1) replace all declarations of 'long' type variable into 'i4'.
**    2) Introduce DT_SI_MAX_TXT_REC and use it to substitue
**       'SI_MAX_TXT_REC'. on i64_hpu, restrict DT_SI_MAX_TXT_REC to
**       SI_MAX_TXT_REC/2. This prevents 'dataldr' from stack overflow.
**    Bug 119835.
*/

#include "stdafx.h"
#include "dtstruct.h"
#include "listchar.h"

void Error(){}
char * x_strstr(const char * lpstring, const char * lpString2Find)
{
   int len=STlength(lpString2Find);
   while(*lpstring !=EOS) {
      if (!STbcompare(lpstring, STlength(lpstring), lpString2Find,len, 1))
         return (char *) lpstring;
      CMnext(lpstring);
   }
   return NULL;
}

/*
** PARSE THE COMMAND LINES
*******************************************************************************
*/
static LISTCHAR* ParseFiles (COMMANDLINE* pDataStructure, LISTCHAR* lc, char* pCurrentToken, int nFileMode, int* nParse)
{
	int nLen = STlength(pCurrentToken);

	if (nParse)
		*nParse = 0;
	if (nFileMode == 1) /* control file */
	{
		if (nLen == 7 && stricmp(pCurrentToken, "control") == 0)
		{
			LISTCHAR* pLookAhead = lc->next;
			if (pLookAhead->pItem && strcmp(pLookAhead->pItem, "=") == 0)
			{
				pLookAhead = pLookAhead->next;
				if (pLookAhead && pLookAhead->pItem)
				{
					int n;
					char* pFile = pLookAhead->pItem;
					if (pFile && *pFile == '\'')
						CMnext (pFile);
					n = STlength(pFile);
					if (n > 0 && pFile[n-1] == '\'')
						pFile[n-1] = '\0';

					CMD_SetControlFile(pDataStructure, pFile);
					if (nParse)
						*nParse = 1;
					return pLookAhead->next;
				}
			}
			else
			{
				Error();
			}
		}
	}
	else
	if (nFileMode == 2) /* log file */
	{
		if (nLen == 3 && stricmp(pCurrentToken, "log") == 0)
		{
			LISTCHAR* pLookAhead = lc->next;
			if (pLookAhead->pItem && strcmp(pLookAhead->pItem, "=") == 0)
			{
				pLookAhead = pLookAhead->next;
				if (pLookAhead && pLookAhead->pItem)
				{
					int n;
					char* pFile = pLookAhead->pItem;
					if (pFile && *pFile == '\'')
						CMnext (pFile);
					n = STlength(pFile);
					if (n > 0 && pFile[n-1] == '\'')
						pFile[n-1] = '\0';

					CMD_SetLogFile(pDataStructure, pFile);
					if (nParse)
						*nParse = 1;
					return pLookAhead->next;
				}
			}
			else
			{
				Error();
			}
		}
	}

	return lc;
}

static LISTCHAR* ParseOptions (COMMANDLINE* pDataStructure, LISTCHAR* lc, char* pCurrentToken, int nFileMode, int* nParse)
{
	int nLen = STlength(pCurrentToken);

	if (nParse)
		*nParse = 0;
	if (nFileMode == 1) /* Parallel */
	{
		if (nLen == 8 && stricmp(pCurrentToken, "Parallel") == 0)
		{
			LISTCHAR* pLookAhead = lc->next;
			if (pLookAhead->pItem && strcmp(pLookAhead->pItem, "=") == 0)
			{
				pLookAhead = pLookAhead->next;
				if (pLookAhead && pLookAhead->pItem)
				{
					CMD_SetParallel(pDataStructure, (tolower(pLookAhead->pItem[0]) == 'y')? 1: 0);
					if (nParse)
						*nParse = 1;
					return pLookAhead->next;
				}
			}
			else
			{
				Error();
			}
		}
	}
	else
	if (nFileMode == 2) /* Direct */
	{
		if (nLen == 6 && stricmp(pCurrentToken, "direct") == 0)
		{
			LISTCHAR* pLookAhead = lc->next;
			if (pLookAhead->pItem && strcmp(pLookAhead->pItem, "=") == 0)
			{
				pLookAhead = pLookAhead->next;
				if (pLookAhead && pLookAhead->pItem)
				{
					CMD_SetDirect(pDataStructure, (tolower(pLookAhead->pItem[0]) == 'y')? 1: 0);
					if (nParse)
						*nParse = 1;
					return pLookAhead->next;
				}
			}
			else
			{
				Error();
			}
		}
	}
	else
	if (nFileMode == 3) /* Readsize */
	{
		if (nLen == 8 && stricmp(pCurrentToken, "readsize") == 0)
		{
			LISTCHAR* pLookAhead = lc->next;
			if (pLookAhead->pItem && strcmp(pLookAhead->pItem, "=") == 0)
			{
				pLookAhead = pLookAhead->next;
				if (pLookAhead && pLookAhead->pItem)
				{
					char* p = pLookAhead->pItem;
					i4 lSize = atol(pLookAhead->pItem);
					while (p && *p)
					{
						if (!CMdigit(p))
						{
							lSize = -1;
							break;
						}
						p = CMnext(p);
					}
					CMD_SetReadSize(pDataStructure, lSize);
					if (nParse)
						*nParse = 1;
					return pLookAhead->next;
				}
			}
			else
			{
				Error();
			}
		}
	}

	return lc;
}


int ParseCommandLine(char* strCommandLine, COMMANDLINE* pDataStructure)
{
	int nError = 0;
	int nControl = 0;
	int nLog = 0;
	int nParallel = 0;
	int nDirect = 0;
	int nReadSize = 0;
	int nVnode= 0;
	int nUflag= 0;

	LISTCHAR* pListToken = NULL;
	pListToken = MakeListTokens(strCommandLine, &nError);
	if (nError != 1 && pListToken)
	{
		/*
		** PARSE COMMAND LINES:
		*/
		LISTCHAR* lc = pListToken;
		while (lc)
		{
			char* pToken = lc->pItem;

			if (nControl == 0 && STlength (pToken) >= 7)
			{
				if (strnicmp (pToken, "control", 7) == 0)
				{
					lc = ParseFiles (pDataStructure, lc, pToken, 1, &nControl);
					if (nControl == 1)
						continue;
				}
			}

			if (nLog == 0 && STlength (pToken) >= 3)
			{
				if (strnicmp (pToken, "log", 3) == 0)
				{
					lc = ParseFiles (pDataStructure, lc, pToken, 2, &nLog);
					if (nLog == 1)
						continue;
				}
			}

			if (nParallel == 0 && STlength (pToken) >= 8)
			{
				if (strnicmp (pToken, "parallel", 8) == 0)
				{
					lc = ParseOptions (pDataStructure, lc, pToken, 1, &nParallel);
					if (nParallel == 1)
						continue;
				}
			}

			if (nDirect == 0 && STlength (pToken) >= 6)
			{
				if (strnicmp (pToken, "direct", 6) == 0)
				{
					lc = ParseOptions (pDataStructure, lc, pToken, 2, &nDirect);
					if (nDirect == 1)
						continue;
				}
			}

			if (nReadSize == 0 && STlength (pToken) >= 8)
			{
				if (strnicmp (pToken, "readsize", 8) == 0)
				{
					lc = ParseOptions (pDataStructure, lc, pToken, 3, &nReadSize);
					if (nReadSize == 1)
					{
						i4 lReadSize = CMD_GetReadSize(&g_cmd);
						if (lReadSize <= 0 || lReadSize > 1000000000)
						{
							SIprintf("The readsize option should be > 0 and less than 1000000000.\n");
							return 0;
						}
						continue;
					}
				}
			}

			if (nUflag == 0 && STlength (pToken) > 2)
			{
				if (strnicmp (pToken, "-u", 2) == 0)
				{
					/* any string begins with -u is considered to be -u<username> */
					nUflag = 1;
					lc = lc->next;
					CMD_SetUFlag(&g_cmd, pToken);
					continue;
				}
			}

			if (nVnode == 0)
			{
				char* pVnode = pToken;
				char* pFound = x_strstr (pVnode, "(local)::");
				if (pFound == pToken)
				{
					int i = 0;
					for (i=0; i<9; i++)
						CMnext (pToken);
				}

				/*
				** Anything that is not key word, we consider it the
				** vnode part. It must be a single token !
				*/
				nVnode = 1;
				CMD_SetVnode (pDataStructure, pToken);
				lc = lc->next;
				continue;
			}

			/*
			** If we reached here, it means that there is a syntax error:
			*/
			nError = 1;
			break;
		}
	}
	ListChar_Done(pListToken);

	return (nError == 1)? 0: 1;
}






/*
** PARSE THE CONTROL FILE
*******************************************************************************
*/
static int aCheck[10];
static int LookAhead (LISTCHAR* listToken, char* strLookupToken, int nLookAhead)
{
	int nCount = ListChar_Count(listToken);
	if (nLookAhead < nCount)
	{
		LISTCHAR* pLookAhead = listToken;
		while (nLookAhead > 0)
		{
			nLookAhead--;
			pLookAhead = pLookAhead->next;
		}
		if (pLookAhead && stricmp (pLookAhead->pItem, strLookupToken) == 0)
			return 1;
		return 0;
	}
	return 0;
}

static void SkipComment (char bufferLine)
{


}

static char* NextNToken (LISTCHAR** listToken, int nNext)
{
	LISTCHAR* lc = *listToken;
	while (lc && nNext > 0)
	{
		lc = lc->next;
		nNext--;
	}
	*listToken = lc;
	if (lc)
		return lc->pItem;
	else
		return NULL;
}

int* GetControlFileMandatoryOptionSet(){return aCheck;}
static LISTCHAR* ParseControlFileLine (DTSTRUCT* pDataStructure, LISTCHAR* listToken, char* strLine);
static int ParseColumnDescription (DTSTRUCT* pDataStructure, LISTCHAR** listToken);
static int IsKeyWord(char* strItem);
int ParseControlFile(DTSTRUCT* pDataStructure, COMMANDLINE* pCmd)
{
	char szRecBuffer[DT_SI_MAX_TXT_REC];
	STATUS st;
	LOCATION lf;
	FIELDSTRUCT* pField = &(pDataStructure->fields);
	FILE* f = NULL;
	LISTCHAR* listToken = NULL;
	char* strControlFile = CMD_GetControlFile(pCmd);
	i4 n = DT_SI_MAX_TXT_REC;
	int nStart = 1;
	int nError = 0;

	if (!strControlFile)
		return 0;

	LOfroms (PATH&FILENAME, strControlFile, &lf);
	st = SIfopen (&lf, "r", SI_TXT, DT_SI_MAX_TXT_REC, &f);
	if (st == OK)
	{
		char* strLine = NULL;
		int nLen = 0;
		/*
		** Read the line from the  control file:
		*/
		while (SIgetrec (szRecBuffer, n, f) != ENDFILE)
		{
			if (!szRecBuffer[0])
				continue;
			strLine = TrimLeft(szRecBuffer);
			if (nStart == 1)
			{
				if (strnicmp (strLine, "LOAD DATA", 9) != 0)
				{
					SIclose(f);
					return 0;
				}
				nStart = 0;
			}

			nLen = STlength(strLine);
			if (nLen > 0)
			{
				if (strLine[nLen -1] == 0x0A)
				{
					if (nLen > 1 && strLine[nLen -2] == 0x0D)
						strLine[nLen -2] = '\0';
					else
						strLine[nLen -1] = '\0';
				}
			}

			listToken = ParseControlFileLine (pDataStructure, listToken, strLine);
		}
		SIclose(f);
	}
	else
	{
		return -1;
	}

	if (listToken)
	{
		LISTCHAR* pLookAhead = NULL;
		LISTCHAR* lc = listToken;
		memset (&aCheck, 0, sizeof(aCheck));
		while (lc)
		{
			char* pCurToken = lc->pItem;
			if (!aCheck[0]) /* check LOAD DATA */
			{
				if (stricmp (pCurToken, "LOAD") == 0)
				{
					lc = lc->next;
					if (stricmp (lc->pItem, "DATA") == 0)
					{
						lc = lc->next;
						aCheck[0] = 1;
						continue;
					}
				}
			}

			if (!aCheck[1]) /* check INFILE */
			{
				/*
				** The ORDER has an important when declaring the INFILE, it should be declared in the following
				** order:
				** {INFILE infilename 
				**     [BADFILE badfilename]
				**     [DISCARDFILE discardfilename]}+
				*/
				if (stricmp (pCurToken, "INFILE") == 0)
				{
					int n;
					char* pFile;
					int badfile = 0;
					int disfile = 0;
					lc = lc->next;
					pFile = lc->pItem;
					if (IsKeyWord(pFile))
					{
						nError = 1; /* Expect a mandatory infile name */
						break;
					}
					if (pFile && *pFile == '\'')
						CMnext (pFile);
					n = STlength(pFile);
					if (n > 0 && pFile[n-1] == '\'')
						pFile[n-1] = '\0';

					lc = lc->next;
					DTSTRUCT_SetInfile (pDataStructure, pFile);

					if (lc)
					{
						if (strnicmp(lc->pItem, "FIX ", 4) == 0)
						{
							int i=0;
							char* slen = lc->pItem;
							for (i=0; i<4; i++)
								CMnext (slen);
							while (slen && (*slen == ' '|| *slen == '\t'))
								CMnext (slen);
							DTSTRUCT_SetFixLength(pDataStructure, atoi(slen));
							lc = lc->next;
						}
					}

					if (lc)
					{
						if (stricmp(lc->pItem, "NULSREPLACEDWITH") == 0)
						{
							lc = lc->next;
							pCurToken = lc->pItem;
							if (stricmp(pCurToken, "SPACE") == 0)
							{
								char c = ' ';
								pCurToken=&c;
							}
							else
							{
								if (STlen(pCurToken)!=1)
								{
									nError = 1; /* Expect 1 character*/
									break;
								}
							}
							DTSTRUCT_SetNullSubstChar(pDataStructure,pCurToken);
							lc = lc->next;
						}
					}
					/* check for optionally specified BADFILE */
					pCurToken = lc->pItem;
					if (stricmp (pCurToken, "BADFILE") == 0)
					{
						int n;
						char* pFile;
						lc = lc->next;
						pFile = lc->pItem;
						if (!IsKeyWord(pFile))
						{
							if (pFile && *pFile == '\'')
								CMnext (pFile);
							n = STlength(pFile);
							if (n > 0 && pFile[n-1] == '\'')
								pFile[n-1] = '\0';
							lc = lc->next;
							DTSTRUCT_SetBadfile (pDataStructure, pFile);
							badfile = 1;
						}
					}
					if (badfile == 0) /* if not specified BADFILE then the default is the INFILE with extention .bad */
					{
						char szBuff[1024];
						char* pFound = NULL;
						char* strInfile = pFile;
						szBuff[0] = '\0';
						if (strInfile)
						{
							char* pTemp;
							STcopy (strInfile, szBuff);
							pFound = STrindex (szBuff, ".", -1);
							while (pFound)
							{
								pTemp = pFound;
								CMnext (pTemp);
								pTemp = STrindex (pTemp, ".", -1);
								if (!pTemp)
									break;
								else
									pFound = pTemp;
							}
							if (pFound)
								*pFound = '\0';

							STcat (szBuff, ".bad");
							DTSTRUCT_SetBadfile(pDataStructure, szBuff);
						}
					}

					/* check for optionally specified DISCARDFILE */
					pCurToken = lc->pItem;
					if (stricmp (pCurToken, "DISCARDFILE") == 0)
					{
						int n;
						char* pFile;
						lc = lc->next;
						pFile = lc->pItem;
						if (!IsKeyWord(pFile))
						{
							if (pFile && *pFile == '\'')
								CMnext (pFile);
							n = STlength(pFile);
							if (n > 0 && pFile[n-1] == '\'')
								pFile[n-1] = '\0';

							lc = lc->next;
							DTSTRUCT_SetDiscardFile (pDataStructure, pFile);
							disfile = 1;
						}
					}

					if (disfile == 0)
					{
						/* nothing to do because we don't handle the DISCARDFILE */
					}
					continue;
				}
			}

			if (!aCheck[2]) 
			{
			}
			if (!aCheck[3])
			{
			}

			if (!aCheck[4]) /* check DISCARDMAX */
			{
				if (stricmp (pCurToken, "DISCARDMAX") == 0)
				{
					i4 lValue;
					lc = lc->next;
					if (IsKeyWord(lc->pItem))
					{
						aCheck[4] = 1;
						continue;
					}
					lValue = atol (lc->pItem);

					lc = lc->next;
					aCheck[4] = 1;
					DTSTRUCT_SetDiscardMax (pDataStructure, lValue);
					continue;
				}
			}

			if (!aCheck[5]) /* check INTO TABLE */
			{
				if (stricmp (pCurToken, "INTO") == 0)
				{
					aCheck[1] = 1; /* INFILE must be declared before this clause */
					lc = lc->next;
					if (stricmp (lc->pItem, "TABLE") == 0)
					{
						lc = lc->next;
						if (IsKeyWord(lc->pItem))
						{
							aCheck[5] = 1;
							continue;
						}
						DTSTRUCT_SetTable  (pDataStructure, lc->pItem);
						lc = lc->next;
						aCheck[5] = 1;
						continue;
					}
				}
				if (STbcompare(pCurToken, 0, "PRESERVE",0,1) == 0)
				{
					aCheck[1] = 1; /* INFILE must be declared before this clause */
					pLookAhead = lc->next;
					if (pLookAhead && STbcompare (pLookAhead->pItem, 0, "BLANKS", 0, 1) == 0)
					{
						pLookAhead = pLookAhead->next;
						if (pLookAhead && STbcompare (pLookAhead->pItem, 0, "INTO", 0, 1) == 0) 
						{
							DTSTRUCT_SetKeepTrailingBlanks(pDataStructure);
							lc = lc->next;
							lc = lc->next;
							continue;
						}
					}
				}
			}

			if (!aCheck[6]) /* check APPEND */
			{
				if (stricmp (pCurToken, "APPEND") == 0)
				{
					aCheck[1] = 1; /* INFILE must be declared before this clause */
					lc = lc->next;
					aCheck[6] = 1;
					continue;
				}
			}

			if (!aCheck[7]) /* check FIELDS TERMINATED BY */
			{
				if (stricmp (pCurToken, "FIELDS") == 0)
				{
					aCheck[1] = 1; /* INFILE must be declared before this clause */
					pLookAhead = lc->next;
					if (pLookAhead && stricmp (pLookAhead->pItem, "TERMINATED") == 0)
					{
						pLookAhead = pLookAhead->next;
						if (pLookAhead && stricmp (pLookAhead->pItem, "BY") == 0)
						{
							pLookAhead = pLookAhead->next;
							if (pLookAhead)
							{
								if (IsKeyWord(pLookAhead->pItem))
								{
									aCheck[7] = 0;
									continue;
								}
								DTSTRUCT_SetFieldTerminator (pDataStructure, pLookAhead->pItem);
								lc = pLookAhead->next;
								if (lc && STbcompare(lc->pItem, 0, "QUOTED",0,1) == 0)
								{
									DTSTRUCT_SetFieldQuoteChar (pDataStructure, "\"");
									lc = lc->next;
								}
								aCheck[7] = 1;
								continue;
							}
						}
					}
				}
			}

			if (!aCheck[8]) /* check COLUMN DESCRIPTION */
			{
				if (stricmp (pCurToken, "(") == 0)
				{
					aCheck[1] = 1; /* INFILE must be declared before this clause */
					aCheck[8] = 1;
					lc = lc->next;
					while (lc)
					{
						if (stricmp (lc->pItem, ")") == 0)
						{
							lc = lc->next;
							break;
						}
						if (!ParseColumnDescription(pDataStructure, &lc))
						{
							nError = 1;
							break;
						}
						if (lc && strcmp (lc->pItem, ",") == 0)
							lc = lc->next;
					}
				}
			}

			/*
			** If we reach here with lc != NULL, it must be an error:
			*/
			if (lc)
				nError = 1;
			if (nError)
				break;
		}
		listToken = ListChar_Done(listToken);
	}

	if (nError == 0)
	{
		int i, nSequenceCount = 0;
		int nLobFileCount = 0;
		int nFillerCount = 0;
		int nColumnCount = 0;
		FIELD* listField = pField->listField;
		while (listField)
		{
				nColumnCount++;
				if (listField->nColAction & COLACT_SEQUENCE)
					nSequenceCount++;
				else
				if (listField->nColAction & COLACT_LOBFILE)
					nLobFileCount++;
				if (listField->nFiller != 0)
					nFillerCount++;

				listField = listField->next;
		}
		pField->pArrayPosition = malloc(nColumnCount * sizeof(int));
		for (i=0; i<nColumnCount; i++)
			pField->pArrayPosition[i] = 0;
		FIELDSTRUCT_SetSequenceCount(pField, nSequenceCount);
		FIELDSTRUCT_SetLobfileCount(pField, nLobFileCount);
		FIELDSTRUCT_SetFillerCount(pField, nFillerCount);
		FIELDSTRUCT_SetColumnCount(pField, nColumnCount);
	}

	return (nError)? 0: 1;
}

static int TokenSeparator(char c)
{
	switch(c)
	{
	case ' ':
	case '\t':
	case ',':
	case '(':
	case ')':
	case ':':
	case '=':
		return 1;
	default:
		return 0;
	}
}

LISTCHAR* ParseControlFileLine (DTSTRUCT* pDataStructure, LISTCHAR* listToken, char* strLine)
{
	char szToken[256];
	char buff[8];
	char stack[256];
	char* pch = strLine;
	char  tchszCur = '0';

	szToken[0] = '\0';
	memset (stack, '\0', sizeof (stack));
	while (pch && *pch != '\0')
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
				}
				else
				{
					/*
					** We consider the current quote the escaped of the next quote (consecutive quotes):
					*/
					STprintf(buff, "%c", tchszCur);
					STcat (szToken, buff);
					STprintf(buff, "%c", PeekNextChar(pch));
					STcat (szToken, buff);
					CMnext(pch);
				}
			}
			else
			{
				STprintf(buff, "%c", tchszCur);
				STcat (szToken, buff);
			}

			CMnext(pch);
			continue;
		} 

		/*
		** We found a token separator:
		*/
		if (TokenSeparator(tchszCur))
		{
			if (szToken[0])
			{
				listToken = AddToken(listToken, szToken);
				szToken[0] = '\0';
			}
			switch(tchszCur)
			{
			case ',':
			case '(':
			case ')':
			case ':':
			case '=':
				STprintf(buff, "%c", tchszCur);
				listToken = AddToken(listToken, buff);
			default:
				break;
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
			else
			if (tchszCur == '-' && PeekNextChar(pch) == '-')
			{
				/* 
				** The comment has been found, skip the whole line
				** from this position:
				*/
				*pch = '\0';
				if (szToken[0])
				{
					listToken = AddToken(listToken, szToken);
					szToken[0] = '\0';
				}
				break;
			}
			else/* Each character is part of the a token */
			{
				STprintf(buff, "%c", tchszCur);
				STcat (szToken, buff);
			}
		}

		CMnext(pch);
		if (*pch == '\0')
		{
			if (szToken[0])
			{
				listToken = AddToken(listToken, szToken);
				szToken[0] = '\0';
			}
		}
	}

	if (stack[0]) /* error: incorrect enclosed characters */
	{
		szToken[0] = '\0';
		listToken = ListChar_Done(listToken);
		return NULL;
	}

	if (szToken[0])
	{
		listToken = AddToken(listToken, szToken);
		szToken[0] = '\0';
	}

	return listToken;
}


int ParseColumnDescription (DTSTRUCT* pDataStructure, LISTCHAR** listToken)
{
	FIELD field;
	FIELDSTRUCT* pField = &(pDataStructure->fields);
	LISTCHAR* lc = *listToken;
	int nTokenCount = ListChar_Count(lc);
	bool	uni = FALSE;

	memset (&field, 0, sizeof(field));
	/*
	** There should be at least 3 tokens:
	*/
	if (nTokenCount < 3)
		return 0;

	STcopy (lc->pItem, field.szColumnName); /* column name */
	lc = lc->next;
	nTokenCount--;

	if (stricmp(lc->pItem, "FILLER")== 0)
	{
		field.nFiller = 1;
		lc = lc->next;
		nTokenCount--;
	}

	if (stricmp(lc->pItem, "POSITION")== 0)
	{
		if (nTokenCount < 7) /* At least: POSITION ( num : num ) DATATYPE , */
		{
			*listToken = lc;
			return 0;
		}

		if (LookAhead (lc, "(", 1) && LookAhead (lc, ":", 3) && LookAhead (lc, ")", 5))
		{
			field.pos1 = atoi (NextNToken(&lc, 2));
			field.pos2 = atoi (NextNToken(&lc, 2));
			lc = lc->next;
			lc = lc->next;
			nTokenCount -= 6;
		}
		else
		{
			*listToken = lc;
			return 0;
		}
	}

	if (nTokenCount < 2) 
	{
		*listToken = lc;
		return 0;
	}

	if (stricmp(lc->pItem, "CHAR")== 0)
	{
		lc = lc->next;
		nTokenCount--;
		if (stricmp (lc->pItem, "NULLIF") == 0)
		{
			field.nColAction |= COLACT_WITHNULL;
			if (nTokenCount < 5) /* At least: NULLIF colname = value ,*/
			{
				*listToken = lc;
				return 0;
			}

			if (LookAhead(lc, field.szColumnName, 1) && LookAhead(lc, "=", 2))
			{
				char* strToken = NextNToken(&lc, 3);
				if (stricmp (strToken, "BLANKS") == 0)
					STcopy ("", field.szNullValue);
				else
				{
					if (stricmp (strToken, ",") == 0)
					{
						if (LookAhead(lc, ",", 1) || LookAhead(lc, "TERMINATED", 1))
							STcopy(strToken, field.szNullValue);
						else
							STcopy("", field.szNullValue);
					}
					else
					if (stricmp (strToken, "TERMINATED") == 0 || stricmp (strToken, ")") == 0)
					{
						STcopy("", field.szNullValue);
					}
					else
					{
						STcopy(TrimLeft(strToken), field.szNullValue);
					}
				}
			}
			else
			{
				*listToken = lc;
				return 0;
			}
			nTokenCount -= 5;
			lc = lc->next;
		}
		else
		if (strnicmp (lc->pItem, "LTRIM", 5) == 0)
		{
			field.nColAction |= COLACT_LTRIM;
			nTokenCount --;
			lc = lc->next;
		}
		field.nDataType = DT_CHAR;
	}
	else
	if (stricmp(lc->pItem, "INTEGER")== 0 && LookAhead (lc, "EXTERNAL", 1))
	{
		lc = lc->next;
		lc = lc->next;
		nTokenCount -= 2;
		if (stricmp (lc->pItem, "NULLIF") == 0)
		{
			field.nColAction |= COLACT_WITHNULL;
			if (nTokenCount < 5) /* At least: NULLIF colname = value ,*/
			{
				*listToken = lc;
				return 0;
			}

			if (LookAhead(lc, field.szColumnName, 1) && LookAhead(lc, "=", 2))
			{
				char* strToken = NextNToken(&lc, 3);
				if (stricmp (strToken, "BLANKS") == 0)
					STcopy ("", field.szNullValue);
				else
				{
					if (stricmp (strToken, ",") == 0)
					{
						if (LookAhead(lc, ",", 1) || LookAhead(lc, "TERMINATED", 1))
							STcopy(strToken, field.szNullValue);
						else
							STcopy("", field.szNullValue);
					}
					else
					if (stricmp (strToken, "TERMINATED") == 0 || stricmp (strToken, ")") == 0)
					{
						STcopy("", field.szNullValue);
					}
					else
					{
						STcopy(TrimLeft(strToken), field.szNullValue);
					}
				}
			}
			else
			{
				*listToken = lc;
				return 0;
			}
			nTokenCount -= 5;
			lc = lc->next;
		}
		field.nDataType = DT_INTEGER;
	}
	else
	if ( (stricmp(lc->pItem, "LOBFILE")== 0) ||
	     (uni = (stricmp(lc->pItem, "LNOBFILE")== 0)) )
	{
		if (nTokenCount < 4) /* At least: LOBFILE (CONTENT_FILE) ,*/
		{
			*listToken = lc;
			return 0;
		}
		if (uni)
		    field.nColAction |= COLACT_LNOB;
		field.nColAction |= COLACT_LOBFILE;
		STcopy (NextNToken(&lc, 2), field.szLobfileName);
		NextNToken(&lc, 2);
		nTokenCount -= 4;
	}
	else
	if (strnicmp(lc->pItem, "to_date", 7)== 0)
	{
		field.nColAction |= COLACT_TODATE;
		nTokenCount --;
		lc = lc->next;
	}
	else
	{
		char* pSeq = x_strstr (lc->pItem, ".NEXTVAL");
		if (pSeq && STlength (pSeq) == 8)
		{
			int len = STlength (lc->pItem);
			field.nColAction |= COLACT_SEQUENCE;
			STlcopy (lc->pItem, field.szSequenceName, len-8);
			nTokenCount --;
			lc = lc->next;
		}
		else
		{
			*listToken = lc;
			return 0;
		}
	}

	/*
	** Column level "TERMINATED BY <string>:
	*/
	if (lc && nTokenCount > 2)
	{
		char* p = lc->pItem;
		if (stricmp(p, "TERMINATED") == 0 && LookAhead(lc, "BY", 1))
		{
			if (nTokenCount < 4) /* At least: TERMINATED BY string ,*/
			{
				*listToken = lc;
				return 0;
			}
			p = NextNToken(&lc, 2);
			lc = lc->next;
			nTokenCount -= 3;
			STcopy(p, field.szTerminator);
			if (lc && STbcompare(lc->pItem, 0, "QUOTED",0,1) == 0)
			{
				nTokenCount --;
				STcopy("\"", field.szQuoting);
				lc = lc->next;
			}
		}
	}

	/*
	** Column level "PRESERVE BLANKS":
	*/
	if (lc && nTokenCount>1)
	{ 
		if (STbcompare(lc->pItem, 0, "PRESERVE",0,1) == 0 && LookAhead(lc, "BLANKS", 1))
		{
			field.iKeepTrailingBlanks = 1;
			lc = lc->next;
			lc = lc->next;
			nTokenCount -= 2;
		}
	}

	*listToken = lc;
	FIELDSTRUCT_AddField(pField, &field);
	return 1;
}

static int IsKeyWord(char* strItem)
{
	if (stricmp (strItem, "INFILE") == 0)
		return 1;
	else
	if (stricmp (strItem, "BADFILE") == 0)
		return 1;
	else
	if (stricmp (strItem, "DISCARDFILE") == 0)
		return 1;
	else
	if (stricmp (strItem, "DISCARDMAX") == 0)
		return 1;
	else
	if (stricmp (strItem, "INTO") == 0)
		return 1;
	else
	if (stricmp (strItem, "TABLE") == 0)
		return 1;
	else
	if (stricmp (strItem, "APPEND") == 0)
		return 1;
	else
	if (stricmp (strItem, "FIELDS") == 0)
		return 1;
	else
	if (stricmp (strItem, "TERMINATED") == 0)
		return 1;
	else
	if (stricmp (strItem, "BY") == 0)
		return 1;
	return 0;

}

