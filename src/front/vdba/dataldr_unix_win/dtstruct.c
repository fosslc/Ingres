
/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dtstruct.c : implementation file
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main structure for data loader
**
** History:
**
** xx-Jan-2004 (uk$so01)
**    Created.
** 03-Mar-2004 (uk$so01)
**    BUG #111895 / Issue 13265678.
**    In data loader, if user specifies the control file without .ctl, 
**    then the program reports an error saying "Failed to create log file."
** 11-Mar-2004 (uk$so01)
**    BUG #111934 / ISSUE 13289717 
**    In data loader, if the space is used as delimiter, the keyword WHITESPACE
**    should be used instead of a blank character ' '.
** 17-Mar-2004 (uk$so01)
**    BUG #111972 / ISSUE 13305051 
**    In data loader, the TERMINATED BY at the column level dit not work as expected.
** 20-Mar-2004 (uk$so01)
**    BUG #112173 / ISSUE 13364045 
**    The data loader loads data incorrectly if user specifies the
**    not consecutive postions for fix length data in the controle file.
** 23-Apr-2004 (uk$so01)
**    DATA LOADER BUG #112187 / ISSUE 13386574 
**    Data loader incorrectly loads data when using BLOB files.
** 24-Apr-2004 (uk$so01)
**    DATA LOADER BUG #112187 / ISSUE 13386574 
**    Data loader incorrectly loads data when using BLOB files.
** 11-Jun-2004 (uk$so01)
**    SIR #111688, additional change to support multiple INFILEs
** 29-Jun-2004 (uk$so01)
**    SIR #111688, additional change to support -u<username> option
**    at the command line.
** 02-Jul-2004 (uk$so01)
**    SIR #111688, additional change to fix some pb with lobfile.
** 12-Jul-2004 (noifr01)
**    SIR #111688, added support of quoting characters, and managing line feeds
**    within the quotes.
** 19-Jul-2004 (noifr01)
**    SIR #111688
**    - added support of PRESERVE BLANKS syntax (both at the global and column
**      level) for keeping trailing spaces at the end of columns
**    - added more specific information in the log file in the case of an error
**      found by dataldr on a row, resulting in dataldr to terminate
** 02-Aug-2004 (noifr01)
**    SIR #111688
**    - open input file in binary mode for supporting EOF (0x1A) character 
**      within the data
**    - check if 0x00 character is found in the data, and provide a specific
**      error message in this case (in non-fix mode, the test is done by 
**      verifying that the last character returned by SIgetrec is 0x0A. If 
**      there is a 0x00 in the data, the EOS "terminated" string won't "reach"
**      that 0x0A)
** 05-Aug-2004 (noifr01)
**    SIR #111688, added support of substitution of NULL character
** 05-Aug-2004 (noifr01)
**    SIR #111688
**    - wrote and use a custom "CachedSIGetRec()" function for
**      performing the equivalent of SIgetRec but using SIRead internally,
**      in order to avoid to use SIGetRec with 0x00 chars before the 0x0A
**      (which seemed to work given that the file is open in binary mode,
**      but given that it doesn't seem to be specifically documented, 
**      the new code should be cleaner
**    - now test for OK status if ENDFILE not returned by SIRead
** 11-Aug-2004 (uk$so01)
**    SIR #111688, additional change:
**    When specifying multiple INFILES, the syntax NULSREPLACEDWITH <'character'> 
**    should be specific to individual INFILE.
**    Example of syntax of INFILE: 
**      (note: if multiple infiles with the same name, then you must specify BADFILE, DISCARDFILE
**             with different name for each infile)
**      INFILE  'dataldr-newfix.txt' "FIX 12" NULSREPLACEDWITH '~'
**      BADFILE 'dataldr-newfix1.bad'
**      INFILE  'dataldr-newfix.txt' "FIX 12" NULSREPLACEDWITH SPACE
**      BADFILE 'dataldr-newfix2.bad'
** 11-Aug-2004 (uk$so01)
**    SIR #111688
**    Additional change to check if SIread returns a value other than
**    ENDFILE and OK as described in Compatlib doc. If so, dataldr will
**    log an error.
** 17-Aug-2004 (uk$so01)
**    SIR #111688
**    Additional change to check if SIread returns a value other than
**    ENDFILE and OK as described in Compatlib doc. If so, dataldr will
**    log an error.
** 23-Sep-2004 (uk$so01)
**    SIR #111688
**    Additional change: add the missing '\n' when writing to log file.
** 15-Dec-2004 (toumi01)
**    Port to Linux.
** 07-May-2005 (hanje04)
**    Quiet pthread compiler warning from gcc
** 06-June-2005 (nansa02)
**    BUG # 114633
**    Fixed memory leak in GetRecordLine improving performance of dataldr.
** 19-Oct-2005 (drivi01)
**    Replaced SI_TEXT with SI_TXT, SI_TEXT is undefined and due to
**    change #479084 SIfopen is defined differently now detecting 
**    undefined SI_TEXT, before it was just ignored.
** 03-Nov-2005 (drivi01)
**    During FormatRecord when final buffer pFiller is being expanded 
**    to fit strRec we also have to account for seqencing later.
** 07-Nov-2005 (drivi01)
**    Removing semi-colon at the end of if statement in the last change.
** 29-jan-2008 (huazh01)
**    1) replace all declarations of 'long' type variable into 'i4'. 
**    2) Introduce DT_SI_MAX_TXT_REC and use it to substitue 
**       'SI_MAX_TXT_REC'. on i64_hpu, restrict DT_SI_MAX_TXT_REC to 
**       SI_MAX_TXT_REC/2. This prevents 'dataldr' from stack overflow.
**    Bug 119835.
**	aug-2009 (stephenb)
**		Prototyping front-ends
*/

#include "stdafx.h"
#include "dtstruct.h"
#include "listchar.h"
#include "sqlcopy.h"
#include "tsynchro.h"
#if defined (LINUX)
#include <unistd.h>
#endif

#include <si.h>
#include <me.h>
#include <er.h>
#define MAXFILEBUF_SIZE 32768
typedef struct tagFILEBUFFER
{
	char  szBuffer[MAXFILEBUF_SIZE+2];
	int   nS;    /* Writting size of buffer, 0: empty */
	int   nR;    /* Reading  size of buffer, nS-nR = 0: no more data to read */
	struct tagFILEBUFFER* next;
} FILEBUFFER;


typedef struct tagCIRCULARBUFFER
{
	int nStartRow;
	FILEBUFFER* pHead;
} CIRCULARBUFFER;

typedef struct tagLOBFILEMGT
{
	char* pFillers;         /* Set of fillers {%02d%04d%s%s}+ = <name len><value len><name><value> */
	char* pc;               /* pointer to the pNormalRecord */
	char* pNormalRecord;    /* normal record */
	FILE* pFile;
	int nFieldIndex;        /* zero-base index lobfile field indicator */

	i4 nReadCount;        /* The amount of bytes read from the buffer file 'pFileBuffer'*/
	i4 nWriteCount;       /* The amount of bytes written to the buffer file 'pFileBuffer'*/
	char  szFileBufferName[1024];
	FILE* pFileBuffer;      /* Temporary file used as buffer: the name will be INFILE + _buf.tmp*/
	int   nForceCommit;     /* Handle the partial commit seperately */
	CIRCULARBUFFER circularBuff;

} LOBFILEMGT;
static short g_nCriticalError = 0;
static i4 nCopyRec = 0;
static i4 nCbCopyBlock = 0;
LOBFILEMGT* pLobfileManageMent = NULL;
LISTCHAR* LOADDATA_FetchRecord(LOADDATA* pLoadData, int* nFinished, int nRemove);


void CIRCULARBUFFER_Init (CIRCULARBUFFER* pObj)
{
	MEfill (sizeof(CIRCULARBUFFER), 0, pObj);
}

void CIRCULARBUFFER_Free (CIRCULARBUFFER* pObj)
{
	FILEBUFFER* pTmp = NULL;
	FILEBUFFER* pFirst = pObj->pHead;
	while (pFirst)
	{
		pTmp = pFirst;
		pFirst = pFirst->next;
		MEfree((PTR)pTmp);
	}
	pObj->nStartRow = 0;
}

int CIRCULARBUFFER_IsEmpty (CIRCULARBUFFER* pObj)
{
	FILEBUFFER* pFirst = pObj->pHead;
	if (!pFirst)
		return 1;
	while (pFirst)
	{
		if ((pFirst->nS == MAXFILEBUF_SIZE) && ((pFirst->nS- pFirst->nR) == 0))
		{
			pObj->pHead = pFirst->next;
			MEfree((PTR)pFirst);
			pFirst = pObj->pHead;
			continue;
		}
		break;
	}

	pFirst = pObj->pHead;
	while (pFirst)
	{
		if ((pFirst->nS - pFirst->nR) > 0)
			return 0;
		pFirst = pFirst->next;
	}
	return 1;
}

int CIRCULARBUFFER_Read (CIRCULARBUFFER* pObj, i4 nSize,i4* lRead, char* pBuffer)
{
	i4 v, n2Read = 0;
	i4 k = nSize;
	char* pSource = NULL;
	char* pDest = pBuffer;
	FILEBUFFER* pFirst = pObj->pHead;

	pBuffer[0] = '\0';
	*lRead = 0;
	while (k > 0)
	{
		pFirst = pObj->pHead;
		/*
		** Remove the first elements in the linked list if they are
		** full and completely.read:
		*/
		while (pFirst)
		{
			if ((pFirst->nS == MAXFILEBUF_SIZE) && ((pFirst->nS- pFirst->nR) == 0))
			{
				pObj->pHead = pFirst->next;
				MEfree((PTR)pFirst);
				pFirst = pObj->pHead;
				continue;
			}
			break;
		}
		/*
		** Fetch element of buffer in the linked list.
		** Always read in the first element in the linked list:
		*/
		pFirst = pObj->pHead;
		while (pFirst)
		{
			if ((pFirst->nS - pFirst->nR) > 0)
				break;
			pFirst = pFirst->next;
		}
		if (!pFirst)
		{
			*lRead = 0;
			return ENDFILE;
		}

		v = pFirst->nS - pFirst->nR;
		n2Read = (k <= v)? k: v;
		k -= n2Read;
		if (k < 0)
			k = 0;

		/*
		** Read 'n2Read' bytes to the buffer of the first element:
		*/
		pSource  = pFirst->szBuffer;
		pSource += pFirst->nR;
		MEcopy (pSource, n2Read, pDest);
		pDest[n2Read] = '\0';
		pDest += n2Read;

		pFirst->nR += n2Read;
		*lRead += n2Read;
	}
	return 0;
}

void CIRCULARBUFFER_Write (CIRCULARBUFFER* pObj, i4 nSize, char* pBuffer)
{
	i4 v, n2Write = 0;
	i4 k = nSize;
	char* pDest = NULL;
	char* pSource = pBuffer;
	FILEBUFFER* pLast = pObj->pHead;

	while (k > 0)
	{
		/*
		** Fetch element of buffer in the linked list.
		** Always write to the last element in the linked list:
		*/
		pLast = pObj->pHead;
		while (pLast && pLast->next)
			pLast = pLast->next;
		if (!pLast)
		{
			pObj->pHead = (FILEBUFFER*)MEreqmem (0, sizeof(FILEBUFFER), 0, NULL);
			MEfill (sizeof(FILEBUFFER), 0, pObj->pHead);
			pLast = pObj->pHead;
		}
		pDest = pLast->szBuffer;
		
		v = MAXFILEBUF_SIZE - pLast->nS;
		if (v <= 0) /* the last element is full, create a new element: */
		{
			FILEBUFFER* pNew = (FILEBUFFER*)MEreqmem (0, sizeof(FILEBUFFER), 0, NULL);
			MEfill (sizeof(FILEBUFFER), 0, pNew);
			pLast->next = pNew;
			pLast = pNew;
			continue;
		}
		n2Write = (k >= v)? v: k;

		k -= n2Write;
		if (k < 0)
			k = 0;

		/*
		** write 'n2Write' bytes to the buffer of the last element:
		*/
		pDest += pLast->nS;
		MEcopy (pSource, n2Write, pDest);
		pDest[n2Write] = '\0';
		pSource += n2Write;

		pLast->nS += n2Write;
	}
}


static void LOBFILEMGT_Init (LOBFILEMGT** pLobMgt)
{
	LOBFILEMGT* pLob = (LOBFILEMGT*)MEreqmem (0, sizeof(LOBFILEMGT), 0, NULL);
	memset (pLob, 0, sizeof(LOBFILEMGT));
	*pLobMgt = pLob;
}

int LOBFILEMGT_IsEmpty(LOBFILEMGT* pLobMgt)
{
	return CIRCULARBUFFER_IsEmpty (&(pLobMgt->circularBuff));
}

int LOBFILEMGT_StartRow(LOBFILEMGT* pLobMgt)
{
	return (pLobMgt->circularBuff.nStartRow == 0)? 1: 0;
}

static void LOBFILEMGT_Free (LOBFILEMGT* pLobMgt, int nAll)
{
	CIRCULARBUFFER_Free (&(pLobMgt->circularBuff));

	if (pLobMgt->pFile)
	{
		SIclose(pLobMgt->pFile);
	}

	if (pLobMgt->pFillers)
	{
		MEfree ((PTR)pLobMgt->pFillers);
		pLobMgt->pFillers = NULL;
	}
	MEfill (sizeof(LOBFILEMGT), 0, pLobMgt);
	if (nAll == 1)
		MEfree ((PTR)pLobMgt);
}

static int LOBFILEMGT_GetLobfileName(LOBFILEMGT* pLobMgt, char* fillerName, char* szLobfileName)
{
	int  n1, n2;
	char szBuffName [64];
	char szBuffVal [1024];

	char* p = pLobMgt->pFillers;
	szLobfileName[0] = '\0';

	/*
	** Do something with 'pFillers' and free it:
	*/
	while (p && *p != '\0')
	{
		STlcopy (p, szBuffName, 2);
		p += 2;
		STlcopy (p, szBuffVal, 4);
		p += 4;

		n1 = atoi(szBuffName);
		n2 = atoi(szBuffVal);
		STlcopy (p, szBuffName, n1);
		p += n1;
		STlcopy (p, szBuffVal, n2);
		p += n2;

		if (stricmp (fillerName, szBuffName) == 0)
		{
			STcopy (szBuffVal, szLobfileName);
			return 1;
		}
	}

	return 0;
}

static int LOBFILEMGT_ConstructLobfiles (LOBFILEMGT* pLobMgt)
{
	char szBuffLen [16];
	LOCATION lf;
	STATUS   st = OK;
	FIELDSTRUCT* pFieldStruct = &(g_dataLoaderStruct.fields);

	char* pRec  = pLobMgt->pNormalRecord;
	if (pRec && pRec[0] == '#')
	{
		/*
		** Record contains the fillers:
		*/
		i4 lFillers = 0;
		char* p = pRec;
		CMnext (p);
		STlcopy (p, szBuffLen, 8);
		lFillers = atol(szBuffLen);
		pLobMgt->pFillers = (char*)MEreqmem (0, lFillers + 1, 0, NULL);
		p = pRec;
		p+= lFillers;
		pLobMgt->pNormalRecord = p;

		pRec+= 9; /* remove '#' and filler len= 8-characters */
		STlcopy (pRec, pLobMgt->pFillers, lFillers);
	}
	pLobMgt->pc = pLobMgt->pNormalRecord;
	pLobMgt->nFieldIndex = 0;
	if (pFieldStruct->pArrayPosition[pLobMgt->nFieldIndex] == 2)
		pLobMgt->nFieldIndex++; /* skip the filler column */

	memset (&lf, 0, sizeof (lf));
	pLobMgt->circularBuff.nStartRow = 1;
	return 1;
}

int LOBFILEMGT_Read  (LOBFILEMGT* pLobMgt, i4 nCount, i4* lRead, char* pBuffer)
{
	int lRes = CIRCULARBUFFER_Read (&(pLobMgt->circularBuff), nCount, lRead, pBuffer);
	return lRes;
}

void LOBFILEMGT_Write (LOBFILEMGT* pLobMgt, i4 nCount, char* pBuffer, i4* dummy)
{
	CIRCULARBUFFER_Write (&(pLobMgt->circularBuff), nCount, pBuffer);
}

void LOBFILEMGT_SIfseek(LOBFILEMGT* pLobMgt, i4 nOffset, i4 nMode)
{
}

/*
** return:
**      -1: error occured
**       0: no more data to read.
**       n: number of bytes read.
*/
i4 LOBFILEMGT_FillBuffer (LOBFILEMGT* pLobMgt, char* lpRow, i4 nlpRowBufferSize)
{
	STATUS   st;
	LOCATION lf;
	int  nLenSpec, nExit;
	char szRecBuffer[4096];
	char szBuffLen  [16];
#if defined (NT_GENERIC)
	char szOpenMode[] = "rb";
#else
	char szOpenMode[] = "r";
#endif
	i4 dummy = 0;
	i4 lByteWindow = nlpRowBufferSize;
	FIELDSTRUCT* pFieldStruct = &(g_dataLoaderStruct.fields);
	int  nFieldCount = FIELDSTRUCT_GetColumnCount(pFieldStruct);
	lpRow[0] = '\0'; /* it is not harmful, it is just for debugging purpose */
	if (nlpRowBufferSize == 0)
	{
		if (LOBFILEMGT_IsEmpty(pLobfileManageMent))
		{
			int nFinished;
			LISTCHAR* pRecord = LOADDATA_FetchRecord(g_pLoadData, &nFinished, 2);
			if (pLobfileManageMent->nForceCommit)
			{
				pLobfileManageMent->nForceCommit = 0;
				return ENDFILE;
			}
			if (!pRecord && nFinished==1)
				return ENDFILE;
			return OK;
		}
		else
			return OK;
	}

	/*
	** Construct the file buffer at least nlpRowBufferSize:
	*/
	nExit = 0;
	while ((lByteWindow > 0) && (nExit != 1))
	{
		nExit = 1;
		/*
		** Check if the current field is a lobfile:
		*/
		if (!pLobMgt->pFile && pFieldStruct->pArrayPosition [pLobMgt->nFieldIndex] == 1)
		{
			char szLobfileName[1024];
			int nBytesCopied = 0;

			nExit = 0;
			STlcopy (pLobMgt->pc, szBuffLen, 5);
			nLenSpec = atoi(szBuffLen);
			pLobMgt->pc += 5;
			STlcopy (pLobMgt->pc, szRecBuffer, nLenSpec);
			pLobMgt->pc += nLenSpec;

			/* szRecBuffer is a filler column name.
			** We find the lobfile name associated in this filler column name:
			*/
			if (szRecBuffer[0]) 
			{
				int nFound = LOBFILEMGT_GetLobfileName(pLobMgt, szRecBuffer, szLobfileName);
				if (nFound == 1 && szLobfileName[0])
				{
					LOfroms (PATH&FILENAME, szLobfileName, &lf);
					st = SIopen (&lf, szOpenMode, &(pLobMgt->pFile));
					if (st !=  OK)
					{
						int nFinished = 0;
						LISTCHAR* pRecord = LOADDATA_FetchRecord(g_pLoadData, &nFinished, 1);
						LOBFILEMGT_Free(pLobMgt, 0);
						if (pRecord)
						{
							if (pRecord->pItem)
				                            free(pRecord->pItem);
							free (pRecord);
							pRecord = NULL;
						}
						WRITE2LOG2("%s: Cannot open file '%s' while trying to handle the BLOB column.\n", INGRES_DATA_LOADER, szLobfileName);
						return -1;
					}
					STcopy (szLobfileName, pLobMgt->szFileBufferName);
				}
				else
				{
					/*
					** Null column of lobfile:
					*/
					LOBFILEMGT_SIfseek(pLobMgt, 0, SI_P_END);
					STcopy ("0 ", szRecBuffer);
					LOBFILEMGT_Write(pLobMgt, 2, szRecBuffer, &dummy);
					pLobMgt->nWriteCount += 2;
					lByteWindow -= 2;
					pLobMgt->nFieldIndex++;
					if (pLobMgt->nFieldIndex < nFieldCount && pFieldStruct->pArrayPosition[pLobMgt->nFieldIndex] == 2)
						pLobMgt->nFieldIndex++; /* skip the filler column */
					if (pLobMgt->nFieldIndex >= nFieldCount)
					{
#if defined (_ADD_NL_)
						STcopy ("\n", szBuffLen); /* end of record */
						LOBFILEMGT_Write(pLobMgt, STlength(szBuffLen), szBuffLen, &dummy);
						pLobMgt->nWriteCount += STlength(szBuffLen);
#endif
						nCopyRec++;
					}
				}
			}
		}

		if (pLobMgt->pFile) /* the record or part of record must be read from file */
		{
			int nContinue = 1;
			i4  lReadSize  = 0;
			nExit = 0;
			while (nContinue)
			{
				st = SIread (pLobMgt->pFile, 4096, &lReadSize, szRecBuffer);
				if (st == ENDFILE || st != OK)
				{
					SIclose(pLobMgt->pFile);
					pLobMgt->pFile = NULL;
					pLobMgt->nFieldIndex++;
					if (pLobMgt->nFieldIndex < nFieldCount && pFieldStruct->pArrayPosition[pLobMgt->nFieldIndex] == 2)
						pLobMgt->nFieldIndex++; /* skip the filler column */
					nContinue = 0;
				}
				if (nContinue == 0 && st != ENDFILE)
				{
					/* Force to write end of BLOB to terminate with this LOB File and log error message: */
					lReadSize = 0; 
					WRITE2LOG2(ERx("%s:READ FILE ERROR while handling lob file '%s'\n"),INGRES_DATA_LOADER, pLobMgt->szFileBufferName);
					return -1;
				}

				LOBFILEMGT_SIfseek(pLobMgt, 0, SI_P_END);
				STprintf (szBuffLen, "%d ", lReadSize);
				LOBFILEMGT_Write(pLobMgt, STlength(szBuffLen), szBuffLen, &dummy);
				pLobMgt->nWriteCount += STlength(szBuffLen);
				if (lReadSize > 0)
				{
					LOBFILEMGT_Write(pLobMgt, lReadSize, szRecBuffer, &dummy);
					pLobMgt->nWriteCount += lReadSize;
					if (nContinue == 0)
					{
						STcopy ("0 ", szBuffLen); /* end of BLOB */
						LOBFILEMGT_Write(pLobMgt, STlength(szBuffLen), szBuffLen, &dummy);
						pLobMgt->nWriteCount += STlength(szBuffLen);
					}
				}
				if (nContinue == 0)
				{
					/*
					** Check also if this is also the end of row!!!
					** If so write one more byte: the '\n'
					*/
					if (pLobMgt->nFieldIndex >= nFieldCount)
					{
#if defined (_ADD_NL_)
						STcopy ("\n", szBuffLen); /* end of record */
						LOBFILEMGT_Write(pLobMgt, STlength(szBuffLen), szBuffLen, &dummy);
						pLobMgt->nWriteCount += STlength(szBuffLen);
#endif
						nCopyRec++;
					}
				}
				lByteWindow -= lReadSize;
				if (lByteWindow <= 0)
					break;
			}
		}
		else /* data must be read from the normal buffer */
		{
			int nBytesCopied = 0;
			if (pLobMgt->pc && *pLobMgt->pc)
			{
				nExit = 0;
				STlcopy (pLobMgt->pc, szBuffLen, 5);
				nLenSpec = atoi(szBuffLen);
				nBytesCopied += (nLenSpec + 5);
				LOBFILEMGT_SIfseek(pLobMgt, 0, SI_P_END);
				LOBFILEMGT_Write(pLobMgt, nBytesCopied, pLobMgt->pc, &dummy);
				pLobMgt->nWriteCount += nBytesCopied;
				pLobMgt->pc += nBytesCopied;
				pLobMgt->nFieldIndex++;
				if (pLobMgt->nFieldIndex < nFieldCount && pFieldStruct->pArrayPosition[pLobMgt->nFieldIndex] == 2)
					pLobMgt->nFieldIndex++; /* skip the filler column */

				lByteWindow -= nBytesCopied;
				/*
				** Check also if this is also the end of row !!!
				** If so write one more byte: the '\n'
				*/
				if (pLobMgt->nFieldIndex >= nFieldCount)
				{
#if defined (_ADD_NL_)
					STcopy ("\n", szBuffLen); /* end of record */
					LOBFILEMGT_Write(pLobMgt, STlength(szBuffLen), szBuffLen, &dummy);
					pLobMgt->nWriteCount += STlength(szBuffLen);
#endif
					nCopyRec++;
				}
			}
			else
			{
				nExit = 1;
			}
		}
	}

	/*
	** Fill the buffer requested by the callback:
	*/
	if (!LOBFILEMGT_IsEmpty(pLobfileManageMent))
	{
		STATUS st;
		i4 lRead = 0;
		LOBFILEMGT_SIfseek(pLobMgt, pLobMgt->nReadCount, SI_P_START);
		st = LOBFILEMGT_Read(pLobMgt, nlpRowBufferSize, &lRead, lpRow);
		pLobMgt->nReadCount += lRead;
		if ((pLobMgt->nFieldIndex >= nFieldCount)  && (pLobMgt->nReadCount == pLobMgt->nWriteCount))
			st = ENDFILE;
		if (st == ENDFILE)
		{
			int nFinished = 0;
			LISTCHAR* pRecord = LOADDATA_FetchRecord(g_pLoadData, &nFinished, 3); /* remove the current record*/
			LOBFILEMGT_Free(pLobMgt, 0);
            LISTCHAR_pRECORD_Free(pRecord);
		}
		lpRow[lRead]='\0'; /* it is not harmful, it is just for debugging purpose */

		return lRead;
	}
	else
		return ENDFILE;

	return 0;
}



/*
** COMMANLINE ARGUMENTS STRUCTURE MANIPULATION
*******************************************************************************
*/
void CMD_SetControlFile(COMMANDLINE* pCmdStructure, char* strFile)
{
	if (strFile)
	{
		int len = STlength(strFile);
		if ( len > 4)
		{
			char* p = strFile;
			p += (len - 4);
			if (stricmp(p, ".ctl") != 0)
			{
				STcopy (strFile, pCmdStructure->szControlFile);
				STcat  (pCmdStructure->szControlFile, ".ctl");
			}
			else
			{
				STcopy (strFile, pCmdStructure->szControlFile);
			}
		}
		else
		{
			STcopy (strFile, pCmdStructure->szControlFile);
			STcat  (pCmdStructure->szControlFile, ".ctl");
		}
	}
}

void CMD_SetLogFile(COMMANDLINE* pCmdStructure, char* strFile)
{
	if (strFile)
		STcopy (strFile, pCmdStructure->szLogFile);
}

void CMD_SetParallel(COMMANDLINE* pCmdStructure, int nSet)
{
	pCmdStructure->nParallel = nSet;
}

void CMD_SetDirect(COMMANDLINE* pCmdStructure, int nSet)
{
	pCmdStructure->nDirect = nSet;
}

void CMD_SetReadSize(COMMANDLINE* pCmdStructure, i4 lReadSize)
{
	pCmdStructure->lReadSize = lReadSize;
}

void CMD_SetVnode (COMMANDLINE* pCmdStructure, char* vNode)
{
	STcopy (vNode, pCmdStructure->szVnode);
}

void CMD_SetUFlag (COMMANDLINE* pCmdStructure, char* uFlag)
{
	if (STlength(uFlag) < 255)
		STcopy (uFlag, pCmdStructure->szUFlag);
}


char* CMD_GetControlFile(COMMANDLINE* pCmdStructure)
{
	return pCmdStructure->szControlFile;
}

char* CMD_GetLogFile(COMMANDLINE* pCmdStructure)
{
	return pCmdStructure->szLogFile;
}

int CMD_GetParallel(COMMANDLINE* pCmdStructure)
{
	return pCmdStructure->nParallel;
}

int CMD_GetDirect(COMMANDLINE* pCmdStructure)
{
	return pCmdStructure->nDirect;
}

i4 CMD_GetReadSize(COMMANDLINE* pCmdStructure)
{
	return pCmdStructure->lReadSize;
}

char* CMD_GetVnode (COMMANDLINE* pCmdStructure)
{
	return pCmdStructure->szVnode;
}

char* CMD_GetUFlag (COMMANDLINE* pCmdStructure)
{
	return pCmdStructure->szUFlag;
}

/*
** FIELD STRUCTURE MANIPULATION
*******************************************************************************
*/
void FIELDSTRUCT_Free(FIELDSTRUCT* pFieldStructure)
{
	FIELD* lpTemp;
	FIELD* ls = pFieldStructure->listField;
	while (ls)
	{
		lpTemp = ls;
		ls = ls->next;
		free ((void*)lpTemp);
	}
	if (pFieldStructure->pArrayPosition)
		free (pFieldStructure->pArrayPosition);
	memset (pFieldStructure, 0, sizeof(FIELDSTRUCT));
}

void FIELDSTRUCT_AddField(FIELDSTRUCT* pFieldStructure, FIELD* pField)
{
	FIELD* lpStruct;
	FIELD* ls = pFieldStructure->listField;

	if (!pField)
		return;
	lpStruct = (FIELD*)malloc (sizeof (FIELD));
	if (!lpStruct)
		return;

	memcpy(lpStruct, pField, sizeof(FIELD));
	lpStruct->next = NULL;
	if (!ls)
	{
		pFieldStructure->listField = lpStruct;
		return;
	}

	while (ls)
	{
		if (!ls->next)
		{
			ls->next = lpStruct; 
			break;
		}
		ls = ls->next;
	}
}

void FIELDSTRUCT_SetColumnCount(FIELDSTRUCT* pFieldStructure, int nCount){pFieldStructure->nCount = nCount;}
void FIELDSTRUCT_SetSequenceCount(FIELDSTRUCT* pFieldStructure, int nCount){pFieldStructure->nSequenceCount = nCount;}
void FIELDSTRUCT_SetLobfileCount(FIELDSTRUCT* pFieldStructure, int nCount){pFieldStructure->nLobFile = nCount;}
void FIELDSTRUCT_SetFillerCount(FIELDSTRUCT* pFieldStructure, int nCount){pFieldStructure->nFiller = nCount;}
int  FIELDSTRUCT_GetColumnCount(FIELDSTRUCT* pFieldStructure){return pFieldStructure->nCount;}
int  FIELDSTRUCT_GetSequenceCount(FIELDSTRUCT* pFieldStructure){return pFieldStructure->nSequenceCount;}
int  FIELDSTRUCT_GetLobfileCount(FIELDSTRUCT* pFieldStructure){return pFieldStructure->nLobFile;}
int  FIELDSTRUCT_GetFillerCount(FIELDSTRUCT* pFieldStructure){return pFieldStructure->nFiller;}



/*
** DTSTRUCT STRUCTURE MANIPULATION
*******************************************************************************
*/
void DTSTRUCT_Free(DTSTRUCT* pDtStructure)
{
	if (pDtStructure->arrayFiles)
		free (pDtStructure->arrayFiles);
	FIELDSTRUCT_Free(&(pDtStructure->fields));
	memset (pDtStructure, 0, sizeof(DTSTRUCT));
}

int DTSTRUCT_SetInfile (DTSTRUCT* pDtStructure, char* strFile)
{
	int nCurFile = pDtStructure->nFileCount;
	pDtStructure->nFileCount++;
	if (pDtStructure->nFileCount > pDtStructure->nArraySize)
	{
		pDtStructure->nArraySize += 5;
		pDtStructure->arrayFiles = realloc (pDtStructure->arrayFiles, pDtStructure->nArraySize);
		if (!pDtStructure->arrayFiles)
			return 0;
	}
	memset (&pDtStructure->arrayFiles[nCurFile], 0, sizeof(INFILES));
	STcopy (strFile, pDtStructure->arrayFiles[nCurFile].infile);
	return 1;
}

int DTSTRUCT_SetBadfile(DTSTRUCT* pDtStructure, char* strFile)
{
	int nCurFile = pDtStructure->nFileCount -1;
	if (nCurFile < 0)
		return 0;
	STcopy (strFile, pDtStructure->arrayFiles[nCurFile].badfile);
	return 1;
}

int DTSTRUCT_SetDiscardFile(DTSTRUCT* pDtStructure, char* strFile)
{
	int nCurFile = pDtStructure->nFileCount -1;
	if (nCurFile < 0)
		return 0;
	STcopy (strFile, pDtStructure->arrayFiles[nCurFile].discardfile);
	return 1;
}

void DTSTRUCT_SetDiscardMax(DTSTRUCT* pDtStructure, i4 lDiscard)
{
	pDtStructure->lDiscardMax = lDiscard;
}

void DTSTRUCT_SetTable  (DTSTRUCT* pDtStructure, char* strTable)
{
	STcopy(strTable, pDtStructure->szTableName);
}

void LISTCHAR_pRECORD_Free(LISTCHAR* pRecord)
{
    if (pRecord)
      {
	   if (pRecord->pItem)
	       {
		  free (pRecord->pItem); 
	       }
       free (pRecord);  
      }  
}

void DTSTRUCT_SetFieldTerminator  (DTSTRUCT* pDtStructure, char* strTerminator)
{
	FIELDSTRUCT* pFields = &(pDtStructure->fields);
	int nLen = STlength(strTerminator);
	if (nLen == 2)
	{
		if (strTerminator[0] == '\\')
		{
			switch (strTerminator[1])
			{
			case 't':
				STcopy ("\t", pFields->szTerminator);
				break;
			case 'n':
				STcopy ("\n", pFields->szTerminator);
				break;
			case '\\':
				STcopy ("\\", pFields->szTerminator);
				break;
			default:
				STcopy (strTerminator, pFields->szTerminator);
				break;
			}
		}
		else
		{
			STcopy (strTerminator, pFields->szTerminator);
		}
	}
	else
	{
		STcopy (strTerminator, pFields->szTerminator);
	}
}

void DTSTRUCT_SetFieldQuoteChar  (DTSTRUCT* pDtStructure, char *strQuoteChar)
{
	/* strQuoteChar is the address of single or double byte char */
	FIELDSTRUCT* pFields = &(pDtStructure->fields);
	char * p = & (pFields->szQuoting[0]);

	CMcpyinc(strQuoteChar,p);
	*p=EOS;
}

void DTSTRUCT_SetNullSubstChar  (DTSTRUCT* pDtStructure, char *strSubstChar)
{
	/* strSubstChar is the address of a single byte char */
	int nCurFile = pDtStructure->nFileCount -1;
	if (nCurFile < 0)
		return;
	pDtStructure->arrayFiles[nCurFile].szNullSubstChar = *strSubstChar;
}

void DTSTRUCT_SetKeepTrailingBlanks (DTSTRUCT* pDtStructure) /* to be called if trailing blanks to be kept for all columns */
{
	FIELDSTRUCT* pFields = &(pDtStructure->fields);
	pFields->iKeepTrailingBlanks = 1;
}

void DTSTRUCT_AddField  (DTSTRUCT* pDtStructure, FIELD* pField)
{
	FIELDSTRUCT* pFields = &(pDtStructure->fields); 
	FIELDSTRUCT_AddField(pFields, pField);
}

int DTSTRUCT_SetFixLength(DTSTRUCT* pDtStructure, int nFixLength)
{
	int nCurFile = pDtStructure->nFileCount -1;
	if (nCurFile < 0)
		return 0;
	pDtStructure->arrayFiles[nCurFile].fixlength = nFixLength;
	return 1;
}
void DTSTRUCT_SetDateFormat(DTSTRUCT* pDtStructure, char* pDateFormat){STcopy (pDateFormat, pDtStructure->szIIdate);}


char* DTSTRUCT_GetInfile (DTSTRUCT* pDtStructure, int num)
{
	if (num >= 0 && num < pDtStructure->nFileCount)
		return pDtStructure->arrayFiles[num].infile[0]? pDtStructure->arrayFiles[num].infile: (char*)0;
	else
		return (char*)0;
}
char* DTSTRUCT_GetBadfile(DTSTRUCT* pDtStructure, int num)
{
	if (num >= 0 && num < pDtStructure->nFileCount)
		return pDtStructure->arrayFiles[num].badfile[0]? pDtStructure->arrayFiles[num].badfile: (char*)0;
	else
		return (char*)0;
}
char* DTSTRUCT_GetDiscardFile(DTSTRUCT* pDtStructure, int num)
{
	if (num >= 0 && num < pDtStructure->nFileCount)
		return pDtStructure->arrayFiles[num].discardfile[0]? pDtStructure->arrayFiles[num].discardfile: (char*)0;
	else
		return (char*)0;
}

i4  DTSTRUCT_GetDiscardMax(DTSTRUCT* pDtStructure){return pDtStructure->lDiscardMax;}
char* DTSTRUCT_GetTable  (DTSTRUCT* pDtStructure){return pDtStructure->szTableName;}
char* DTSTRUCT_GetFieldTerminator  (DTSTRUCT* pDtStructure)
{
	FIELDSTRUCT* pFields = &(pDtStructure->fields);
	return pFields->szTerminator;
}

char* DTSTRUCT_GetFieldQuoteChar  (DTSTRUCT* pDtStructure)
{
	FIELDSTRUCT* pFields = &(pDtStructure->fields);
	return pFields->szQuoting;
}

char* DTSTRUCT_GetNullSubstChar    (DTSTRUCT* pDtStructure, int num)
{
	if (num >= 0 && num < pDtStructure->nFileCount)
		return &(pDtStructure->arrayFiles[num].szNullSubstChar);
	return 0;
}

int   DTSTRUCT_GetKeepTrailingBlanks (DTSTRUCT* pDtStructure)
{
	FIELDSTRUCT* pFields = &(pDtStructure->fields);
	return pFields->iKeepTrailingBlanks;
}

int DTSTRUCT_ColumnLevelFieldTerminator(DTSTRUCT* pDtStructure)
{
	FIELDSTRUCT* pFields = &(pDtStructure->fields);
	FIELD* ls = pFields->listField;
	while (ls)
	{
		if (!ls->szTerminator[0])
			return 0;
		ls = ls->next;
	}

	return 1;
}

int DTSTRUCT_GetFixLength(DTSTRUCT* pDtStructure, int num)
{
	if (num >= 0 && num < pDtStructure->nFileCount)
		return pDtStructure->arrayFiles[num].fixlength;
	return 0;
}
char* DTSTRUCT_GetDateFormat(DTSTRUCT* pDtStructure){return pDtStructure->szIIdate;}






int LOADDATA_IsFinished(LOADDATA* pLoadData)
{
	if (pLoadData->nEnd == 1)
		return 1;
	return 0;
}

void LOADDATA_Finish(LOADDATA* pLoadData)
{
	pLoadData->nEnd = 1;
}

void LOADDATA_AddRecord(LOADDATA* pLoadData, char* strRecord)
{
	LISTCHAR* lpStruct;

	if (!strRecord)
		return;
	lpStruct = (LISTCHAR*)malloc (sizeof (LISTCHAR));
	if (!(strRecord && lpStruct))
		return;

	lpStruct->next = NULL;
	lpStruct->prev = NULL;
	lpStruct->pItem = strRecord;

	if (!pLoadData->pHead)
		pLoadData->pHead = lpStruct;
	else
	if (pLoadData->pTail)
	{
		pLoadData->pTail->next = lpStruct;
		lpStruct->prev = pLoadData->pTail;
	}

	pLoadData->pTail = lpStruct;
}

void LOADDATA_Free(LOADDATA* pLoadData)
{
	if (pLoadData)
	{
		pLoadData->pHead = ListChar_Done(pLoadData->pHead);
		memset (pLoadData, 0, sizeof(LOADDATA));
	}
}


/*
** Return 1: Success
**        0: no more field or fail
*/
static int GetNextField(DTSTRUCT* pDataStructure, FIELD* pField, char** pCur, char* szOutBuffer)
{
	char szField[DT_SI_MAX_TXT_REC];
	char strTeminator[16];
	char strQuoting[16];
	char* pch = *pCur;
	char  tchszCur = '0';
	char* pFound = NULL;
	char* strField = NULL;
	int   i = 0;
	int   nEnclosed = 0; /* 1= double quote <">, 2= simple quote <'> */
	int   nSepLen = 0;
	int   nQuotLen= 0;
	int   iQuotedAndCopied;
	int   iKeepTrailingBlanks;

	STcopy (DTSTRUCT_GetFieldTerminator(pDataStructure), strTeminator);
	szField[0] = '\0';
	szOutBuffer[0] = '\0';
	if (pField->szTerminator[0])
		STcopy(pField->szTerminator, strTeminator); /* override by the column level */
	nSepLen = STlength(strTeminator);

	STcopy (DTSTRUCT_GetFieldQuoteChar(pDataStructure), strQuoting);
	if (pField->szQuoting[0])
		STcopy(pField->szQuoting, strQuoting); /* overriden at the column level */
	nQuotLen = STlength(strQuoting);

	iKeepTrailingBlanks=DTSTRUCT_GetKeepTrailingBlanks(pDataStructure);
	 /* no "negative override" for this parameter. Override ony */
	/* to set if not set at the global level */
	if (!iKeepTrailingBlanks)
		iKeepTrailingBlanks = pField->iKeepTrailingBlanks;

	if (pField->nColAction & COLACT_SEQUENCE)
	{
		/*
		** Skip the sequence. It will be generated during the SQL COPY precessing:
		*/
		return 1;
	}

	if (nEnclosed == 0)
	{
		/*
		** Use the POSITION to fetch the field:
		*/
		if (pField->pos1 > 0 && pField->pos2 > 0)
		{
			int nCharCount = pField->pos2 - pField->pos1 +1;
			STlcopy (pch + (pField->pos1-1), szField, nCharCount);
		}
		else
		{
			/*
			** Use the separator to fetch the field:
			*/
			if (!strTeminator[0])
			{
				WRITE2LOG1("%s: No field terminator found.\n", INGRES_DATA_LOADER);
				return 0;
			}

			/*
			** For the LOBFILE, strTeminator must be the EOF:
			*/
			if (pField->nColAction & COLACT_LOBFILE)
			{

				return 1;
			}

			iQuotedAndCopied = 0;
			if (nQuotLen && pch && *pch==strQuoting[0]) { /* quoted string */
				i4 ilen2copy = DT_SI_MAX_TXT_REC-1;
				char *pInField = szField;

				CMnext(pch); /* after initial quote*/

				while (1)
				{
					if (*pch==strQuoting[0])
					{
						CMnext(pch); 
						if (*pch!=strQuoting[0])
							break;
					}
					if (*pch==EOS) 
					{
						return 0;
					}
					ilen2copy -=CMbytecnt(pch); 
					CMcpyinc(pch,pInField);
				}
				*pInField=EOS;
				iQuotedAndCopied = 1;
				if (*pch==0x0D || *pch==0x0A || *pch==EOS) { /*End of line outside quotes can't belong to data itself ... */
					*pCur = NULL;
				}
				else
				{
					if (stricmp (strTeminator, "WHITESPACE") != 0) /* normal (non whitespace) separator */
					{
						if (STbcompare(strTeminator,STlen(strTeminator),pch,0,0)==0) {
							for (i=0; i<nSepLen; i++)
								CMnext (pch);
							*pCur = pch;
						}
						else
						{
							if (*pch!=EOS)
								return 0;
							*pCur = NULL;
						}
					}
					else /* WHITESPACE as separator: */
					{
						if (*pch!=' ' && *pch!='\t')
						{
							if (*pch!=EOS)
								return 0;
							*pCur = NULL;
						}
						else 
						{
							while (pch && *pch != EOS)
							{
								if (*pch == ' ' || *pch == '\t')
								{
									CMnext (pch);
								}
								else
									break;

							}
							*pCur = pch;
						}
					}
				}
			}

			if (iQuotedAndCopied==0) {
				if (stricmp (strTeminator, "WHITESPACE") != 0)
				{
					pFound = STindex (pch, strTeminator, -1);
					if (pFound)
					{
						pFound[0] = '\0';
						for (i=0; i<nSepLen; i++)
							CMnext (pFound);
						STlcopy (pch, szField, DT_SI_MAX_TXT_REC-1);
						*pCur = pFound;
					}
					else
					if (pch)
					{
						int nLen = 0;
						STlcopy (pch, szField, DT_SI_MAX_TXT_REC-1);
						/*
						** This is the last field. The 0x0A or 0x0D,0x0A must be removed:
						*/
						nLen = STlength(szField);
						if (nLen > 0)
						{
							if (szField[nLen -1] == 0x0A)
							{
								if (nLen > 1 && szField[nLen -2] == 0x0D)
									szField[nLen -2] = '\0';
								else
									szField[nLen -1] = '\0';
							}
						}

						*pCur = NULL;
					}
				}
				else /* WHITESPACE as separator: */
				{
					int nCharCount = 0;
					char* pColText = pch;
					while (pch && *pch != '\0')
					{
						if (*pch == ' ' || *pch == '\t')
						{
							while (pch && (*pch == ' ' || *pch == '\t'))
								CMnext (pch);
							break;
						}
						nCharCount++;
						CMnext (pch);
					}
					if (nCharCount > 0)
						STlcopy (pColText, szField, nCharCount);
					else
						STcopy ("", szField);

					*pCur = pch;
				}
			} /* iQuotedAndCopied==0 */
		}
	}
	else
	{
#if 0 /* The posibility of field enclosed by quote or double quote is not specified in DDS */
		char buff[8];
		char stack[256];
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
						STprintf(buff, "%c", tchszCur);
						STcat (szField, buff);
					}
					else
					{
						/*
						** We consider the current quote the escaped of the next quote (consecutive quotes):
						*/
						STprintf(buff, "%c", tchszCur);
						STcat (szField, buff);
						STprintf(buff, "%c", PeekNextChar(pch));
						STcat (szField, buff);
						CMnext(pch);
					}
				}
				else
				{
					STprintf(buff, "%c", tchszCur);
					STcat (szField, buff);
				}

				CMnext(pch);
				continue;
			} 

			/*
			** We found a field separator:
			*/
			if (strnicmp(strTeminator, pch, nSepLen) == 0)
			{
				for (i=0; i<nSepLen; i++)
					CMnext (pch);
				*pCur = pch;
				break;
			}
			else
			{
				if ((nEnclosed == 1 && tchszCur == '\"') || (nEnclosed == 2 && tchszCur == '\'')) /* Found the begin quote.*/
				{
					/*
					** Note: the begin quote cannote be an escape of quote because 
					** the quote can be escaped only if it is inside the quote:
					*/
					stack_PushState (stack, tchszCur);
				}
				else/* Each character is part of the a fieald */
				{
					STprintf(buff, "%c", tchszCur);
					STcat (szField, buff);
				}
			}

			CMnext(pch);
		}

		if (stack[0]) /* error */
		{
			szField[0] = '\0';
			return 0;
		}
#endif
	}

	if (pField->nColAction & COLACT_LTRIM)
		strField = TrimLeft(szField);
	else
		strField = szField;
	if (iKeepTrailingBlanks==0)
		TrimRight(strField);
	STcopy (strField, szOutBuffer);

	return 1;
}

/*
** This function used in addition the the record length.
** It is nescessary because in case of the load table has only sequence
** then the record length is 0. And thus a reader thread writes 0 size for a record
** that has only SEQUENCES in the QUEUE will never get read by the loader thread!
*/
int GetBufferSize4Sequence(int nSequenceCount)
{
	/* Because the sequence is not known, just reserve the approximate buffer size:
	** 20 bytes for a sequence field (5 for length specifier + 15 for value witch is 999999999999999 max)
	** and that is enought !!
	*/
	int nSize = nSequenceCount*20;
	return nSize;
}

static int isRecordComplete(DTSTRUCT* pDataStructure, char* pdatabuf, int* nError)
{
	FIELDSTRUCT* pField = &(pDataStructure->fields);
	FIELD* listField = pField->listField;
	i4 nLen = 0;
	int nFieldCount=0;
	char* p = pdatabuf;
	char strTeminator[16];
	char strQuoting[16];
	int   nSepLen;
	int   nQuotLen;
	int i;

	while (listField)
	{
		if (listField->nColAction & COLACT_SEQUENCE ||
			listField->nColAction & COLACT_LOBFILE 
		   )
		{   /* calculated columns, not part of read data */
			listField = listField->next;
			continue;
		}

		STcopy (DTSTRUCT_GetFieldTerminator(pDataStructure), strTeminator);
		if (listField->szTerminator[0])
			STcopy(listField->szTerminator, strTeminator); /* overriden at the column level */
		nSepLen = STlength(strTeminator);

		STcopy (DTSTRUCT_GetFieldQuoteChar(pDataStructure), strQuoting);
		if (listField->szQuoting[0])
			STcopy(listField->szQuoting, strQuoting); /* overriden at the column level */
		nQuotLen = STlength(strQuoting);

		if (strQuoting[0]==EOS || 
			(strQuoting[0]!=EOS && strQuoting[0]!=*p) 
		   ) {
			if (stricmp (strTeminator, "WHITESPACE") != 0)
			{
				p = STindex (p, strTeminator, -1);
				if (p)
				{
					for (i=0; i<nSepLen; i++)
						CMnext (p);
				}
				else
					return (1); /* assumes end of record */
			}
			else /* WHITESPACE as separator: */
			{
				while (p && *p != EOS)
				{
					if (*p == ' ' || *p == '\t')
					{
						while (p && (*p == ' ' || *p == '\t'))
							CMnext (p);
						break;
					}
					CMnext (p);
				}
				if (*p==EOS)
					return (1);
			}
		}
		else
		{ /* column has started with quoting chars */
			while (1)
			{
				CMnext(p);
				if (*p == strQuoting[0]) {
					CMnext(p);
					if (*p == strQuoting[0]) {
						/* escaped quote */
						continue;
					}
					/* we are after the trailing quote, should find here the separator, or be at the end of line*/
					if (*p==0x0D || *p==0x0A || *p==EOS) /*End of line outside quotes can't belong to data itself ... */
						return 1;

					if (stricmp (strTeminator, "WHITESPACE") != 0) /* normal (non whitespace) separator */
					{
						if (STbcompare(strTeminator,STlen(strTeminator),p,0,0)==0) {
							for (i=0; i<nSepLen; i++)
								CMnext (p);
							break;
						}
						/* neither end of line, nor separator*/
						/* fatal error, no need to continue, FormatRecord should record the fatal error*/
						*nError = 1;
						return 1;
					}
					else /* WHITESPACE as separator: */
					{
						if (*p!=' ' && *p!='\t')
							return ERROR;
						while (p && *p != EOS)
						{
							if (*p == ' ' || *p == '\t')
							{
								CMnext (p);
							}
							else
								break;

						}
					}
					if (*p==EOS)
						return (1);

					listField = listField->next;
					continue;

				}
				else { /* character within quotes */
					if (*p==EOS) /*End of data within quotes means there is not enough data ... */
						return 0;
				}
			}
		}

		listField = listField->next;
	}
	return 1;
}

static char* FormatRecord(DTSTRUCT* pDataStructure, char* szRecBuffer, int nFieldCount, int SeqCount, int* nError)
{
	char szField[DT_SI_MAX_TXT_REC];
	i4  nSize = STlength(szRecBuffer) + nFieldCount*5 + 2 + GetBufferSize4Sequence(SeqCount);
	char* strRec = malloc (nSize);
	FIELDSTRUCT* pField = &(pDataStructure->fields);
	FIELD* listField = pField->listField;
	if (strRec)
	{
		char szIIDate[32];
		char szBufferLenSpec[32];
		i4 nLen = 0;
		int nFieldCount=0;
		char* p = szRecBuffer;
		char* pFiller = NULL;
		strRec[0] = '\0';
		STcopy (DTSTRUCT_GetDateFormat(pDataStructure), szIIDate);
		while (listField)
		{
			if (listField->nColAction & COLACT_SEQUENCE)
			{
				/*
				** The sequence field is calculated, so do not read the field
				** from the record string:
				*/
				listField = listField->next;
				continue;
			}

			if (listField->nColAction & COLACT_LOBFILE)
			{
				/*
				** The LOBFILE field is calculated, so do not read the field
				** from the record string:
				*/
				nLen = STlength (listField->szLobfileName);
				STprintf (szField, "%5d%s", nLen, listField->szLobfileName);
				if ((i4)(STlength(strRec) + 5 + nLen) >= nSize)
				{
					nSize += (5 + nLen + 1);
					strRec = realloc (strRec, nSize);
				}
				STcat (strRec, szField);
				listField = listField->next;
				continue;
			}

			if (GetNextField(pDataStructure, listField, &p, szField))
			{
				nFieldCount++;
				nLen = 0;
				/*
				** Need to transform the field?
				*/
				if (szField[0])
				{
					if ((listField->nColAction & COLACT_TODATE) && szIIDate[0])
					{
						char szDate[64];
						STcopy (szField, szDate);
						/*
						** Convert the date to the input formart of 'szIIDate':
						*/
						INGRESII_DateConvert(szIIDate, szDate, szField);
					}
					nLen = STlength (szField);
				}

				if (listField->nFiller != 1)
				{
					STprintf (szBufferLenSpec, "%5d", nLen);
					if ((i4)(STlength(strRec) + 5 + STlength(szField)) >= nSize)
					{
						nSize += (5 + STlength(szField) + 1);
						strRec = realloc (strRec, nSize);
					}
					STcat (strRec, szBufferLenSpec);
					if (szField[0])
						STcat (strRec, szField);
				}
				else
				{
					/*
					** Read the filler field but the data is not put in the record
					** to be loaded. The data of the FILLER fields will be put at the begining of the
					** record. Each filler is specified by:
					**   6-bytes (2-bytes for filler name length and 4-bytes for value length)
					**   the string )<filler name><filler value> of length equals to (name length + value length):
					*/
					char szFillerBuff [2000];
					int nLenFillerCol = STlength(listField->szColumnName);
					int nLenFillerVal = STlength(szField);
					STprintf (szFillerBuff, "%02d%04d%s%s", nLenFillerCol, nLenFillerVal, listField->szColumnName, szField);
					if (!pFiller)
					{
						pFiller = malloc (STlength(szFillerBuff) + 9 + 1);
						STcopy ("#00000000", pFiller);
						STcat (pFiller, szFillerBuff);
					}
					else
					{
						pFiller = realloc(pFiller,  STlength(pFiller) + STlength(szFillerBuff) +1);
						STcat (pFiller, szFillerBuff);
					}
				}
/*
				if (!p)
					break;
*/
			}
			else
			{
				strRec[0] = '\0';
				if (nError)
					*nError = 1;
				break;
			}
			listField = listField->next;
		}

		if (nError && *nError == 1)
		{
			if (pFiller)
			{
				free (pFiller);
				pFiller = NULL;
			}
		}
		if (pFiller)
		{
			/*
			** Filler is put at the begining of the record as follow:
			** First byte is '#' indicating that record has filler
			** Next 8-bytes are the length-specifier of the set of filler columns
			*/
			int i, nFillerSize = STlength(pFiller);
			STprintf (szBufferLenSpec, "%08d", nFillerSize);
			for (i=0; i<8; i++)
				pFiller[i+1] = szBufferLenSpec[i];

			pFiller = realloc (pFiller, nFillerSize + STlength(strRec) + 1 + GetBufferSize4Sequence(SeqCount));
			STcat (pFiller, strRec);
			free (strRec);
			strRec = pFiller;
		}
	}

	return strRec;
}


static int iCacheInUse=0;
static char bufcache[DT_SI_MAX_TXT_REC];
static i4 i4ReadInCache;
static i4 i4WriteInCache;
static i4 iEndFound;

static STATUS	initReadCache()
{
	if (iCacheInUse>0)
		return FAIL;
	iCacheInUse = 1;
	i4ReadInCache = 0;
	i4WriteInCache = 0;
	iEndFound = 0;
	return OK;
}
static STATUS	freeReadCache()
{
	if (iCacheInUse==0)
		return FAIL; /* error*/
	iCacheInUse = 0;
	return OK;
}

STATUS CachedSIgetrec(char *  p, i4 n, FILE *  f)
{
	i4 lRead; 
	STATUS sr;
	i4 lToRead;
	int i;
	if (i4WriteInCache > i4ReadInCache)
	{
		for (i=i4ReadInCache;i<=i4WriteInCache;i++)
		{
			int len=i-i4ReadInCache;
			if ( len == n-1)
			{
				memcpy(p,(void *)(bufcache+i4ReadInCache),len);
				*(p+len)='\0';
				i4ReadInCache+=len;
				return OK;
			}

			if (i<i4WriteInCache && bufcache[i]==0x0A) 
			{
				len++; /* now includes current char (0x0A)*/
				memcpy(p,(void *)(bufcache+i4ReadInCache),len);
				*(p+len)='\0';
				i4ReadInCache+=len;
				return OK;
			}
		}
	}
	if (iEndFound>0)
		return ENDFILE;

	/* use memmove rather than memcpy for avoiding buffer overlap issues */
	if (i4ReadInCache>0) /* otherwise data already there, if any */
		memmove(&(bufcache[0]),&(bufcache[0])+i4ReadInCache,(i4WriteInCache - i4ReadInCache));

	i4WriteInCache-=i4ReadInCache;
	lToRead = sizeof(bufcache) - i4WriteInCache;
	sr = SIread (f, lToRead, &lRead, &(bufcache[0])+i4WriteInCache);
	if (sr==FAIL || lRead>lToRead)
		return FAIL;
	if (sr==ENDFILE)
		iEndFound=1;
	i4WriteInCache+=lRead;
	i4ReadInCache = 0;
	for (i=i4ReadInCache;i<=i4WriteInCache;i++)
	{
		int len=i-i4ReadInCache;
		if ( len == n-1)
		{
			memcpy(p,(void *)(bufcache+i4ReadInCache),len);
			*(p+len)='\0';
			i4ReadInCache+=len;
			return OK;
		}

		if (i<i4WriteInCache && bufcache[i]==0x0A) 
		{
			len++; /* now includes current char (0x0A)*/
			memcpy(p,(void *)(bufcache+i4ReadInCache),len);
			*(p+len)='\0';
			i4ReadInCache+=len;
			return OK;
		}
	}
	return FAIL; /* normally because n > than our buffer */
}

STATUS SIgetrec0(char *  p, i4 n, FILE *  f, int* nError, char *pNullSubstChar, i4* pnNullSubst)
{
	STATUS status = CachedSIgetrec (p, n, f);
	if (status==OK)
	{
		if (*pNullSubstChar == 0x00)
		{
			int nLen = STlength(p);
			if (nLen > 0)
			{
				if (*(p + (nLen -1)) != 0x0A)
					*nError=1;
			}
		}
		else
		{
			int nLen;
			while (1)
			{
				nLen=STlength(p);
				if (nLen==0  || 
					(nLen>0 && nLen<n-1 && (*(p + (nLen -1)) != 0x0A) )
				   )
				{
					*(p+nLen)=*pNullSubstChar;
					(* pnNullSubst)++;
				}
				else
					break;
			}
		}
	}
	return (status);
}
static STATUS mySIgetrec (int iCanHaveMultipleLines, DTSTRUCT*pDataStructure, char * szRecBuffer, int buflen, i4 n, FILE *f, int* nError, i4* pnNullSubst)
{
	STATUS status ;
	int error = 0;
	int ilen;
	char * p = szRecBuffer;
	char * pNullSubstChar = DTSTRUCT_GetNullSubstChar(pDataStructure, pDataStructure->nCurrent);
	i4 i2read = n;
	if (i2read>buflen)
		i2read=buflen;

	*pnNullSubst = 0;

	if (iCanHaveMultipleLines)
	{
		while (1)
		{
			status = SIgetrec0 (p, n, f, nError, pNullSubstChar, pnNullSubst);
			if (*nError)
				return status;
			if (status != OK)
				return status;
			if (isRecordComplete(pDataStructure, szRecBuffer, &error))
				return (status);
			ilen=STlength(p);
			p+=ilen;
			buflen-=ilen;
			if (i2read>buflen)
				i2read=buflen;
			if (buflen<1)
				break;
		} 
		return (status);
	}
	else {
		status = SIgetrec0 (szRecBuffer, n, f, nError, pNullSubstChar, pnNullSubst);
		return (status);
	}
}

/*
** Reader thread:
*/
static void ftThreadReadInputFile(DTSTRUCT* pDataStructure)
{
	STATUS st, sr;
	char szRecBuffer[DT_SI_MAX_TXT_REC+1];
	char szRecBuffer1[DT_SI_MAX_TXT_REC+1];
	char* strLine = NULL;
#if defined (NT_GENERIC)
	char szOpenMode[] = "rb";
#else
	char szOpenMode[] = "r";
#endif
	FILE* f = NULL;
	int nError = 0;

	/*
	** Open the file for reading:
	*/
	char* pInfile = DTSTRUCT_GetInfile(pDataStructure, pDataStructure->nCurrent);
	if (pInfile)
	{
		LOCATION lf;
		i4 nRowsWithNullChars = 0;
		i4 nTotalNullSubstChars = 0;
		i4 iSubstNullChars = 0;

		i4 n = DT_SI_MAX_TXT_REC;

		if (* (DTSTRUCT_GetNullSubstChar(pDataStructure, pDataStructure->nCurrent)) !=0x00)
			iSubstNullChars = 1;

		LOfroms (PATH&FILENAME, pInfile, &lf);

		st = SIfopen (&lf, szOpenMode, SI_TXT, DT_SI_MAX_TXT_REC, &f);
		if (st == OK)
		{
			FIELDSTRUCT* pField = &(pDataStructure->fields);
			/*
			** Calculate the number of records:
			*/
			int nRecCount = FIELDSTRUCT_GetColumnCount(pField);
			int nSequenceCount = FIELDSTRUCT_GetSequenceCount(pField);
			i4 lFix = DTSTRUCT_GetFixLength(pDataStructure, pDataStructure->nCurrent);
			/*
			** Read the record from the input file, format the record to 
			** the accepted record by the copy into program:
			*/
			if (lFix > 0)
			{
				i4 lRead = 0;
				i4 lCurRecord = 0;
				while (1)
				{
					nError = 0;
					sr = SIread (f, lFix, &lRead, szRecBuffer);
					if (sr == ENDFILE && lRead == 0)
						break;
					if (sr != OK && sr!=ENDFILE)
					{
						WRITE2LOG1(ERx("%s:READ FILE ERROR\n"),INGRES_DATA_LOADER);
						break;
					}
					szRecBuffer[lFix]=EOS;
					if (lRead != lFix)
					{
						STprintf(szRecBuffer1,ERx("File Read Error on Record %ld"),lCurRecord);
						nError = 1;
						break;
					}
					STcopy(szRecBuffer,szRecBuffer1); /* copy for error display */
					if (iSubstNullChars) {
						i4 HasNullChars = 0;
						i4 ilen;
						while (1)
						{
							ilen = STlen(szRecBuffer);
							if (ilen < lFix) 
							{
								szRecBuffer[ilen]= * (DTSTRUCT_GetNullSubstChar(pDataStructure, pDataStructure->nCurrent));
								HasNullChars = 1;
								nTotalNullSubstChars++;
							}
							else
								break;
						}
						if (HasNullChars)
							nRowsWithNullChars++;
					}
					else 
					{
						if (STlen(szRecBuffer)!=lFix)
						{
							WRITE2LOG2(ERx("%s: NULL character found in record %ld\n"), INGRES_DATA_LOADER, lCurRecord);
							nError = 1;
							break;
						}
					}
					strLine = FormatRecord(pDataStructure, szRecBuffer, nRecCount, nSequenceCount, &nError);
					if (nError)
						break;
					/*
					** Wait for writing condition:
					*/
					WaitForBuffer(&g_ReadSizeEvent, STlength(strLine) + GetBufferSize4Sequence(nSequenceCount));
					/*
					** Wait for the mutex:
					*/
					AccessQueueWrite (&g_mutexLoadData, strLine, 1);
					
					lCurRecord++;
				}
				if ((sr != OK && sr!=ENDFILE)|| nError)
				{
					nError = 1;
				}
			}
			else
			{
				STATUS st1;
				int iCanHaveMultipleLines = 0;
				i4 iNullChars;
				char * ptmp = DTSTRUCT_GetFieldQuoteChar(pDataStructure);
				if (*ptmp !=EOS)
					iCanHaveMultipleLines = 1;
				else 
				{
					FIELDSTRUCT* pField = &(pDataStructure->fields);
					FIELD* CurField = pField->listField;
					while (CurField)
					{
						if (CurField->szQuoting[0])
						{
							iCanHaveMultipleLines = 1;
							break;
						}

						CurField = CurField->next;
					}
				}

				st1 =initReadCache();
				if (st1!=OK)
				{
					WRITE2LOG1(ERx("%s:READ FILE INTERNAL ERROR \n"),INGRES_DATA_LOADER);
				}
				else
				{
					st1 = OK;
					while ((st1 = mySIgetrec (iCanHaveMultipleLines, pDataStructure, szRecBuffer, sizeof (szRecBuffer)-1, n, f, &nError,&iNullChars)) == OK)
					{
						int nLen;
						if (nError)
						{
							WRITE2LOG1("%s: NULL character found in file.\n", INGRES_DATA_LOADER);
							STcopy(szRecBuffer,szRecBuffer1); /* copy for error display */
							break;
						}
						if (iNullChars>0)
						{
							nRowsWithNullChars ++;
							nTotalNullSubstChars += iNullChars;
						}
						nLen = STlength(szRecBuffer);
						if (nLen > 0)
						{
							if (szRecBuffer[nLen -1] == 0x0A)
							{
								if (nLen > 1 && szRecBuffer[nLen -2] == 0x0D)
									szRecBuffer[nLen -2] = '\0';
								else
									szRecBuffer[nLen -1] = '\0';
							}
						}
						nError = 0;
						if (!szRecBuffer[0])
							continue;
						STcopy(szRecBuffer,szRecBuffer1); /* copy for error display */
						strLine = FormatRecord(pDataStructure, szRecBuffer, nRecCount, nSequenceCount, &nError);
						if (nError)
							break;
						/*
						** Wait for writing condition:
						*/
						WaitForBuffer(&g_ReadSizeEvent, STlength(strLine) + GetBufferSize4Sequence(nSequenceCount));
						/*
						** Wait for the mutex:
						*/
						AccessQueueWrite (&g_mutexLoadData, strLine, 1);
					}
					if (st1 != OK && st1!=ENDFILE)
					{
						WRITE2LOG1(ERx("%s:READ FILE ERROR\n"),INGRES_DATA_LOADER);
					}
					st1= freeReadCache();
					if (st1!=OK)
					{
						WRITE2LOG1(ERx("%s:READ FILE INTERNAL ERROR 2\n"),INGRES_DATA_LOADER);
					}
					if (st1 != OK || nError)
					{
						nError = 1;
					}
				}
			}

			SIclose(f);
			if (nRowsWithNullChars>0 || nTotalNullSubstChars>0)
			{
				WRITE2LOG1(ERx("%s:"),INGRES_DATA_LOADER);
				WRITE2LOG3(ERx("%ld NULL character(s) were replaced by '%c' in a total of %ld row(s)\n"), 
				    nTotalNullSubstChars, (* (DTSTRUCT_GetNullSubstChar(pDataStructure, pDataStructure->nCurrent))), nRowsWithNullChars);
			}
			if (nError) 
			{
				WRITE2LOG1("%s: There was an error while reading the input file\n", INGRES_DATA_LOADER);
				WRITE2LOG1("%s: Invalid data:\n", INGRES_DATA_LOADER);
				WRITE2LOG0(szRecBuffer1);
				WRITE2LOG0("\n");
			}
		}
		else
		{
			WRITE2LOG2("%s: Cannot open data file '%s'\n", INGRES_DATA_LOADER, pInfile);
		}
	}
	if (nError)
	{
		char chErr[10];
		STcopy ("Error", chErr);
		AccessQueueWrite (&g_mutexLoadData, chErr, 0); /* Notify Writer thread of an error */
	}
	else
	{
		AccessQueueWrite (&g_mutexLoadData, NULL, 0); /* signal end of file */
	}

#if defined (UNIX)
	{
		STATUS status;
		pthread_exit (&status);
	}
#else
	_endthread();
#endif
}

void StartInputReaderThread(DTSTRUCT* pDataStructure)
{
#if defined (UNIX)
	pthread_t p_id = 0;
	pthread_create (&p_id, pthread_attr_default, (void *)ftThreadReadInputFile, pDataStructure);
#else
	_beginthread(ftThreadReadInputFile, 0, (void*)pDataStructure);
#endif
}

/*
** i (must be > 0) is a the th-matched sequence field in the list:
*/
static int GetSequence (DTSTRUCT* pDataStructure, int i, char* szNextValue)
{
	/*
	** Get the sequence name:
	*/
	char* pName = NULL;
	FIELDSTRUCT* pField = &(pDataStructure->fields);
	FIELD* listField = pField->listField;
	while (listField)
	{
		if (!(listField->nColAction & COLACT_SEQUENCE))
		{
			listField = listField->next;
			continue;
		}

		i--;
		if (i == 0)
		{
			pName = listField->szSequenceName;
			break;
		}
		listField = listField->next;
	}

	if (pName && pName[0])
	{
		/*
		** Use the SQL statement to get the Sequence number:
		*/
		int nVal = INGRESII_llSequenceNext(pName, szNextValue);
		return nVal;
	}

	return 0;
}


/*
** Param: nRemove 
**    1: nomal record, fetch and remove
**    2: check if there is a record only
**    3: remove the current record if any
**    0: same as 1) but not remove the record.
*/
LISTCHAR* LOADDATA_FetchRecord(LOADDATA* pLoadData, int* nFinished, int nRemove)
{
	if (nFinished)
		*nFinished = 0;
	/*
	** Wait for the mutex:
	*/
	if (AccessQueueRead(&g_mutexLoadData))
	{
		/*
		** Read data from the QUEUE:
		*/
		int i;
		int len;
		int nExtra4Sequence;
		char bufSeq   [128];
		char bufSeqVal[128];
		i4 lSize = 0;
		LISTCHAR* pData = pLoadData->pHead;
		if (pLoadData->nEnd == -1) /* SIread returns an error */
		{
			UnownMutex(&g_mutexLoadData);
			if (nFinished)
				*nFinished = -1;
			return NULL;
		}
		if (!pLoadData->pHead)
		{
			if (LOADDATA_IsFinished(pLoadData) == 1)
			{
				if (nFinished)
					*nFinished = 1;
			}
			UnownMutex(&g_mutexLoadData);
			return NULL;
		}
		if (nRemove == 2)
		{
			UnownMutex(&g_mutexLoadData);
			return pData;
		}

		/*
		** Cause the COPY INTO to commit by returning NULL (ENDOFDATA):
		*/
		if ((nRemove != 3) && nCbCopyBlock >= g_lReadSize)
		{
			if (pLobfileManageMent)
				pLobfileManageMent->nForceCommit = 1;
			UnownMutex(&g_mutexLoadData);
			return NULL;
		}

		if (pData)
		{
			FIELDSTRUCT* pField = &(g_dataLoaderStruct.fields);
			int nSequenceColumnCount = FIELDSTRUCT_GetSequenceCount(pField);
			if (nFinished)
				*nFinished = 0;

			if (nRemove == 1 || nRemove == 3)
			{
				if (pData == pLoadData->pTail)
				{
					pLoadData->pHead = NULL;
					pLoadData->pTail = NULL;
				}
				else
				{
					pLoadData->pHead = pData->next;
					if (pLoadData->pHead)
						pLoadData->pHead->prev = NULL;
				}
				if (nRemove == 3)
				{
					UnownMutex(&g_mutexLoadData);
					return pData;
				}
			}

			/*
			** The buffer size is exclude the sequence part because
			** during the read process the sequence is unknown, and the thread just
			** reserved the approx buffer size of 'GetBufferSize4Sequence(nSequenceCount)':
			*/
			lSize = STlength(pData->pItem) + GetBufferSize4Sequence(nSequenceColumnCount);
			nCbCopyBlock += lSize;
			lSize = -lSize;
			WaitForBuffer(&g_ReadSizeEvent, lSize);
			/*
			** Generate the sequence on the fly:
			** If there are N sequences then the buffer is reserved with the call to
			** GetBufferSize4Sequence(N)-> extra size for sequence.
			*/
			nExtra4Sequence = GetBufferSize4Sequence(nSequenceColumnCount);
			for (i=0; i<nSequenceColumnCount; i++)
			{
				int nOk = GetSequence (&g_dataLoaderStruct, i+1, bufSeqVal);
				if (nOk == 0)
				{
					char* pError = INGRESII_llGetLastSQLError();
					if (pError && pError[0])
					{
						WRITE2LOG1("%s\n", pError);
					}
					STcopy ("0", bufSeqVal);
				}
				len = STlength(bufSeqVal);
				STprintf (bufSeq, "%5d%s", len, bufSeqVal);
				len = STlength(bufSeq);
				nExtra4Sequence -= len;
				if (nExtra4Sequence <= 0)
				{
					pData->pItem = realloc (pData->pItem, STlength(pData->pItem) + len + 2);
					nExtra4Sequence += len;
				}
				STcat (pData->pItem, bufSeq);
			}

			UnownMutex(&g_mutexLoadData);
			return pData;
		}
		else
		{
			if (LOADDATA_IsFinished(pLoadData))
			{
				if (nFinished)
					*nFinished = 1;
			}
			UnownMutex(&g_mutexLoadData);
			return NULL;
		}
	}

	/*
	** Cannot own the mutex:
	*/
	if (nFinished)
		*nFinished = -1;
	return NULL;
}



/*
** GetRecordLine:
** 
** Summary: CALLBACK for SQL COPY
** Parameters:
**    rowLength : maximum number of bytes that can be returned
**    lpRow     : pointer to a contiguous block of memory at least rowLength 
**                in size in which to return record(s)
**    returnByte: number of bytes actually returned in the buffer.
** RETURN VALUE: 
**     0: successful outcome
**     1: failed
**  9900: (ENDFILE). End of data reached. This should be accompanied with a returnByte value of 0
*/
i4 GetRecordLine (i4* rowLength, char* lpRow, i4* returnByte)
{
	int   nAddedNL  = 0;
	int   nFinished = 0;
	char* strRecord = NULL;
	LISTCHAR* pRecord = NULL;
#if defined (_ADD_NL_)
	nAddedNL = 1;
#endif

	g_nGetRecordLineIvoked = 1;
	/*
	** Get record from the QUEUE but not remove it. This current record will be pointed by the structure 
	** 'pLobfileManageMent' and the subsequence call, the callback use this pointer instead of accessing the Q.
	** When the callback finishes with the current record, the current record will removed from the Q 
	** and set pLobfileManageMent to NULL.
	*/
	if (pLobfileManageMent)
	{
		if (!pLobfileManageMent->pNormalRecord)
			pRecord = LOADDATA_FetchRecord(g_pLoadData, &nFinished, 0);
		if (nFinished == -1)
		{
			*returnByte  = 0;
			g_nCriticalError = 1;
			return 1;  /* FAIL */
		}
		if (pRecord)
		{
			pLobfileManageMent->pNormalRecord = pRecord->pItem;
			if (LOBFILEMGT_StartRow(pLobfileManageMent))
				LOBFILEMGT_ConstructLobfiles (pLobfileManageMent);
		}
	}
	else
	{
		pRecord = LOADDATA_FetchRecord(g_pLoadData, &nFinished, 1);
		if (nFinished == -1)
		{
			*returnByte  = 0;
			g_nCriticalError = 1;
			return 1;  /* FAIL */
		}

		if (!pRecord)
		{
			*returnByte  = 0;
			return 9900; /* ENDFILE */
		}
	}

	if (pLobfileManageMent) /* Handle the lobfile */
	{
		i4 lRequest = *rowLength;
		i4 lReturnBytes = LOBFILEMGT_FillBuffer (pLobfileManageMent, lpRow, lRequest);
		*returnByte = 0;
		if (lReturnBytes == -1)
		{
			g_nCriticalError = 1;
			return 1;
		}
		if (lRequest == 0)
			return lReturnBytes;
		*returnByte = lReturnBytes;
		if (lReturnBytes < lRequest)
		{
			return 9900;
		}
	}
	else
	{
		/*
		** 12-Feb-2004: (begin)-->
		** Add block of code to support also the FILLER (dummy column even if no BLOB associated
		** with this filler column.) Just skip the field data
		*/
		FIELDSTRUCT* pField = &(g_dataLoaderStruct.fields);
		int nFillerCount = FIELDSTRUCT_GetFillerCount(pField);
		char* pRec = pRecord->pItem;
		strRecord = pRec;
		if (nFillerCount > 0)
		{
			if (pRec && pRec[0] == '#')
			{
				/*
				** Record contains the fillers:
				*/
				char szBuffLen [16];
				i4 lFillers = 0;
				char* p = pRec;
				CMnext (p);
				STlcopy (p, szBuffLen, 8);
				lFillers = atol(szBuffLen);
				p = pRec;
				p+= lFillers;
				pRec = p;
			}
			strRecord = pRec;
		}
		/*
		** 12-Feb-2004: (end)<--
		*/

		*returnByte  = STlength(strRecord) + nAddedNL;
		if (*rowLength < *returnByte)
		{
            LISTCHAR_pRECORD_Free(pRecord);
			g_nCriticalError = 1;
			return 1;
		}

		nCopyRec++;
		STcopy (strRecord, lpRow);
		if (nAddedNL)
			STcat (lpRow, "\n");
        LISTCHAR_pRECORD_Free(pRecord);
	}

	return 0;
}



/*
** Writer thread:
*/
static i4 lCopiedRows = 0;
static i4 lTotalCopiedRows = 0;
int StartTransferDataThread(DTSTRUCT* pDataStructure)
{
	char szRecBuffer[DT_SI_MAX_TXT_REC+1];
	char szTempFile [1024];
	int nOk = -1;
	int nParallel = CMD_GetParallel(&g_cmd);
	char* strCopyStatement = NULL;

	int nCommit = 0;
	int nContinue = 1;
	char* strBadFile = DTSTRUCT_GetBadfile(pDataStructure, pDataStructure->nCurrent);
	FILE* tmpFile = NULL;
	if (!g_pLoadData)
		return nOk;
	if (pDataStructure->nCurrent == 0)
		g_lCpuStarted = INGRESII_llGetCPUSession();
	if (FIELDSTRUCT_GetLobfileCount(&(g_dataLoaderStruct.fields)) > 0)
	{
		LOBFILEMGT_Init(&pLobfileManageMent);
	}
	/*
	** Generate the copy into syntax:
	*/
	strCopyStatement = GenerateCopySyntax(pDataStructure, pDataStructure->nCurrent);
	if (!strCopyStatement)
	{
		return nOk;
	}
#if defined (_SHOW_COPYTABLE_SYNTAX)
	WRITE2LOG1("SYNTAX of copy table:\n%s\n", strCopyStatement);
#endif
	TRACE1("SYNTAX of copy table:\n%s\n", strCopyStatement);

	nOk = 1;
	INGRESII_llSetCopyHandler((PfnIISQLHandler)GetRecordLine);
	while (nContinue)
	{
		g_nGetRecordLineIvoked = 0;
		/*
		** Run the sql copy until the GetRecordLine returns ENDOFDATA:
		*/
		nCommit = 0;
		nOk = INGRESII_llExecuteCopyFromProgram (strCopyStatement, &lCopiedRows);
		/*
		** Because of each execution of the COPY ... FROM PROGRAM will create the bad file
		** and write eoors to it (if any), then we must concatenate those bad file into the final
		** single bad file. 
		** First, write the content of the successive bad file to the temporary file "tmpFile":
		*/
		if (strBadFile && access(strBadFile, 0) != -1)
		{
			STATUS st;
			LOCATION lf;
			FILE* f1;
			i4 lRead = 0;
			i4 n = DT_SI_MAX_TXT_REC;
			if (!tmpFile)
			{
				STcopy (strBadFile, szTempFile);
				STcat  (szTempFile, ".tmp");
				LOfroms (PATH&FILENAME, szTempFile, &lf);
				/*
				** I open the file in SI_TXT mode and use SIwrite because in SI_BIN I don't know
				** what is the appropriate value of the 4th parameter to be passed. It is not explained
				** in the Compatlib documentation
				*/ 
				st = SIfopen (&lf, "w", SI_TXT, DT_SI_MAX_TXT_REC, &tmpFile);
			}

			LOfroms (PATH&FILENAME, strBadFile, &lf);
			/*
			** I open the file in SI_TXT mode and use SIread because in SI_BIN I don't know
			** what is the appropriate value of the 4th parameter to be passed. It is not explained
			** in the Compatlib documentation
			*/ 
			st = SIfopen (&lf, "r", SI_TXT, DT_SI_MAX_TXT_REC, &f1);
			if (st == OK && tmpFile && f1)
			{
				int nDumy =0;
				while (1)
				{
					st = SIread (f1, DT_SI_MAX_TXT_REC, &lRead, szRecBuffer);
					if (st == ENDFILE && lRead == 0)
						break;
					SIwrite (lRead, szRecBuffer, &nDumy, tmpFile);
				}
				SIclose (f1);
			}
		}

		lTotalCopiedRows += lCopiedRows;
		if (LOADDATA_IsFinished(g_pLoadData))
			nContinue = g_pLoadData->pHead? 1: 0;

		if (nOk == 0)
			break;
		if (g_nCriticalError != 0)
			break;
		/*
		** Partial commit:- commit the some copied rows:
		*/
		if ((nCbCopyBlock >= g_lReadSize) || nContinue == 0)
		{
			nCommit = 1;
			nCbCopyBlock = 0;
			INGRESII_llCommitSequence();
			INGRESII_llExecuteImmediate ("commit");
			SIprintf("Commit point reached - logical record count %d\n", nCopyRec);
		}
		if (nContinue && nParallel && nCommit)
		{
			nOk = INGRESII_llSetCopyHandler((PfnIISQLHandler)NULL);
			if (nOk == 0)
				break;
			nOk = INGRESII_llExecuteImmediate ("commit");
			if (nOk == 0)
				break;
			/*
			** Set the COPY handler again:
			*/
			nOk = INGRESII_llSetCopyHandler((PfnIISQLHandler)GetRecordLine);
			if (nOk == 0)
				break;
		}
	}
	INGRESII_llSetCopyHandler((PfnIISQLHandler)NULL);
	free (strCopyStatement);

	/*
	** Because of each execution of the COPY ... FROM PROGRAM will create the bad file
	** and write eoors to it (if any), then we must concatenate those bad file into the final
	** single bad file. 
	** Second, read the content of the temporary file and write to the Bad File:
	*/
	if (tmpFile && strBadFile)
	{
		STATUS st;
		LOCATION lf;
		FILE* f = NULL;
		i4 lRead = 0;
		
		SIclose(tmpFile);
		LOfroms (PATH&FILENAME, szTempFile, &lf);
		/*
		** I open the file in SI_TXT mode and use SIread because in SI_BIN I don't know
		** what is the appropriate value of the 4th parameter to be passed. It is not explained
		** in the Compatlib documentation
		*/ 
		st = SIfopen (&lf, "r", SI_TXT, DT_SI_MAX_TXT_REC, &tmpFile);
		if (st == OK && tmpFile)
		{
			LOfroms (PATH&FILENAME, strBadFile, &lf);
			/*
			** I open the file in SI_TXT mode and use SIwrite because in SI_BIN I don't know
			** what is the appropriate value of the 4th parameter to be passed. It is not explained
			** in the Compatlib documentation
			*/ 
			st = SIfopen (&lf, "w", SI_TXT, DT_SI_MAX_TXT_REC, &f);
			if (st == OK)
			{
				while (1)
				{
					st = SIread (tmpFile, DT_SI_MAX_TXT_REC, &lRead, szRecBuffer);
					if (st == ENDFILE && lRead == 0)
						break;
					SIwrite(lRead, szRecBuffer, &lRead, f);
				}
				SIclose(f);
			}
			SIclose(tmpFile);
			LOfroms (PATH&FILENAME, szTempFile, &lf);
			st = LOdelete(&lf);
		}
	}

	if (FIELDSTRUCT_GetLobfileCount(&(g_dataLoaderStruct.fields)) > 0 && pLobfileManageMent)
	{
		LOBFILEMGT_Free(pLobfileManageMent, 1);
		pLobfileManageMent = NULL;
	}

	if ((g_dataLoaderStruct.nCurrent + 1) == g_dataLoaderStruct.nFileCount)
	{
		g_lCpuEnded = INGRESII_llGetCPUSession();
		WRITE2LOG0("\n\n\n");
		WRITE2LOG1("Table %s:\n", DTSTRUCT_GetTable(pDataStructure));
		WRITE2LOG1("  %d Rows successfully loaded.\n", lTotalCopiedRows);
		WRITE2LOG1("  %d Rows not loaded due to data errors.\n", nCopyRec-lTotalCopiedRows);
		WRITE2LOG1("  %d Rows not loaded because all WHEN clauses were failed.\n", 0);
		WRITE2LOG1("  %d Rows not loaded because all fields were null.\n", 0);
		
		TRACE1 ("Read Record       =%05d\n", g_lRecordRead);
		TRACE1 ("Record QUEUE      =%05d\n", ListChar_Count(g_pLoadData->pHead));
		TRACE1 ("Record Copied     =%05d\n", nCopyRec);
	}
	return nOk;
}

void Write2Log (FILE* f, char* strLine)
{
	i4 count = 0;
	i4 len = STlength(strLine);
	if (!f)
		return;
	if (!strLine)
		return;
	if (OwnMutex (&g_mutexWrite2Log))
	{
		SIwrite(len, strLine, &count, f);
		UnownMutex(&g_mutexWrite2Log);
	}
}



