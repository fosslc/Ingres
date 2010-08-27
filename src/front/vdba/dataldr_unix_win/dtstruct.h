/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dtstruct.h : header file
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main structure for data loader
**
** History:
**
** xx-Dec-2003 (uk$so01)
**    Created.
** 01-Mar-2004 (uk$so)
**    BUG #11187, In addtion to the log file, syntax errors detected 
**    in the control file in the data loader should also be reported in the stdout.
** 23-Apr-2004 (uk$so01)
**    DATA LOADER BUG #112187 / ISSUE 13386574 
**    Data loader incorrectly loads data when using BLOB files.
** 11-Jun-2004 (uk$so01)
**    SIR #111688, additional change to support multiple INFILEs
** 29-Jun-2004 (uk$so01)
**    SIR #111688, additional change to support -u<username> option
**    at the command line.
** 12-Jul-2004 (noifr01)
**    SIR #111688, added support of quoting characters, and managing line feeds
**    within the quotes.
** 19-Jul-2004 (noifr01)
**    SIR #111688, added support of PRESERVE BLANKS syntax, for keeping
**    trailing spaces at the end of columns
** 05-Aug-2004 (noifr01)
**    SIR #111688, added support of substitution of NULL character
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
** 15-Dec-2004 (toumi01)
**    Port to Linux.
** 06-June-2005 (nansa02)
**    BUG # 114633
**    Fixed memory leak in GetRecordLine improving performance of dataldr.
** 12-May-2006 (thaju02)
**    Added COLACT_LNOB, to identify long nvarchar. 
** 29-jan-2008 (huazh01)
**    1) replace all declarations of 'long' type variable into 'i4'.
**    2) Introduce DT_SI_MAX_TXT_REC and use it to substitue
**       'SI_MAX_TXT_REC'. on i64_hpu, restrict DT_SI_MAX_TXT_REC to
**       SI_MAX_TXT_REC/2. This prevents 'dataldr' from stack overflow.
**    Bug 119835.
**	5-Aug-2010 (kschendel) b124197
**	    Update for long names;  since dataldr doesn't use iicommon (!!!)
**	    it has to be hand coded here.
*/

#if !defined (MAIN_DATALOADER_STRUCTURE_HEADER)
#define MAIN_DATALOADER_STRUCTURE_HEADER
#include "listchar.h"
#if defined (UNIX)
#include <stdio.h>
#include <sys/file.h>
#endif
#include <compat.h>

/* B119835 */
#if defined (i64_hpu)
#define DT_SI_MAX_TXT_REC SI_MAX_TXT_REC / 2 
#else
#define DT_SI_MAX_TXT_REC SI_MAX_TXT_REC
#endif

extern i4 g_lReadSize;
extern i4 g_lRecordRead;
extern int  g_nGetRecordLineIvoked;
extern i4 g_lCpuStarted;
extern i4 g_lCpuEnded;

#define INGRES_DATA_LOADER "INGRES DATA Loader"
#define READSIZE_DEF     65536
#define EXTRA 4
#define DT_CHAR 1
#define DT_INTEGER (DT_CHAR + 1)
#if defined (UNIX)
#define ERROR 1
#endif

#define COLACT_WITHNULL  0x00000001
#define COLACT_LTRIM     0x00000002
#define COLACT_TODATE    0x00000004
#define COLACT_SEQUENCE  0x00000008
#define COLACT_LOBFILE   0x00000010
#define COLACT_LNOB      0x00000020

#define DB_ATT_MAXNAME	256
#define DB_TAB_MAXNAME	256
#define DB_OWN_MAXNAME	32
#define DB_SEQ_MAXNAME	256

typedef struct tagFIELD
{
	char szColumnName [DB_ATT_MAXNAME + EXTRA];
	char szTerminator[16];   /* if this one is set, it override the one of table level */
	int  nDataType;          /* DT_CHAR, DT_INTEGER */
	int  nFiller;            /* 1: FILLER key word is set */
	int  pos1;
	int  pos2;
	int  nColAction;         /* COLACT_WITHNULL, COLACT_LTRIM, COLACT_TODATE */
	char szNullValue[64];    /* valide only if bWithNULL = 1. NULL Condition */
	char szSequenceName[DB_SEQ_MAXNAME+1]; /* valide only if (nColAction & COLACT_SEQUENCE) != 0  */
	char szLobfileName[1024];/* valide only if (nColAction & COLACT_LOBFILE) != 0  */
	char szQuoting[16];      /* if set, overrides the table level one */
	int  iKeepTrailingBlanks;/* keep traiing blanks if not 0 */
	struct tagFIELD* next;
} FIELD;

typedef struct tagFIELDSTRUCT
{
	char szTerminator[16];   /* field terminated by */
	int  nSequenceCount;     /* if non-zero, a number of columns defined as SEQUENCE */
	int  nLobFile;           /* if non-zero, a number of columns defined as LOBFILE  */
	int  nCount;             /* Total number of columns */
	int  nFiller;            /* Total number of filler columns */
	int* pArrayPosition;     /* Position of column in the copy table... [value= (0=normal, 1=lobfile, 2=Filler)]*/
	char szQuoting[16];      /* quoting character, empty string if no quoting */
	int  iKeepTrailingBlanks;/* keep traiing blanks if not 0 */
	FIELD* listField;
} FIELDSTRUCT;

typedef struct tagINFILES
{
	char infile[1024];       /* MANDATORY, might be inclosed by quote character */
	char badfile[1024];      /* might be inclosed by quote character */
	char discardfile[1024];  /* might be inclosed by quote character */
	int  fixlength;          /* FIX record length */
	char szNullSubstChar;    /* character to replace 0x00 */
} INFILES;

typedef struct tagDTSTRUCT
{
	int nArraySize;          /* The size of arrayFiles (default = 5) */
	int nFileCount;          /* Number of elements in the arrayFiles */
	int nCurrent;            /* The current INFILE being handled */
	INFILES* arrayFiles;     /* list of {INFILE, [BADFILE], [DISCARDFILE]}*/
	char  szTableName[DB_TAB_MAXNAME+DB_OWN_MAXNAME+1+1];  /* in the format [owner.]tableName */
	i4  lDiscardMax;
	int   nFixLength;
	FIELDSTRUCT fields;
	char  szIIdate[32];      /* empty: no need to convert date, otherwise covert to the acceptable input */

} DTSTRUCT;

extern DTSTRUCT g_dataLoaderStruct;
void FIELDSTRUCT_Free(FIELDSTRUCT* pFieldStructure);
void FIELDSTRUCT_AddField(FIELDSTRUCT* pFieldStructure, FIELD* pField);
void FIELDSTRUCT_SetColumnCount(FIELDSTRUCT* pFieldStructure, int nCount);
void FIELDSTRUCT_SetSequenceCount(FIELDSTRUCT* pFieldStructure, int nCount);
void FIELDSTRUCT_SetLobfileCount(FIELDSTRUCT* pFieldStructure, int nCount);
void FIELDSTRUCT_SetFillerCount(FIELDSTRUCT* pFieldStructure, int nCount);
int  FIELDSTRUCT_GetColumnCount(FIELDSTRUCT* pFieldStructure);
int  FIELDSTRUCT_GetSequenceCount(FIELDSTRUCT* pFieldStructure);
int  FIELDSTRUCT_GetLobfileCount(FIELDSTRUCT* pFieldStructure);
int  FIELDSTRUCT_GetFillerCount(FIELDSTRUCT* pFieldStructure);

void DTSTRUCT_Free(DTSTRUCT* pDtStructure);
int  DTSTRUCT_SetInfile (DTSTRUCT* pDtStructure, char* strFile);
int  DTSTRUCT_SetBadfile(DTSTRUCT* pDtStructure, char* strFile);
int  DTSTRUCT_SetDiscardFile(DTSTRUCT* pDtStructure, char* strFile);
void DTSTRUCT_SetDiscardMax(DTSTRUCT* pDtStructure, i4 lDiscard);
void DTSTRUCT_SetTable  (DTSTRUCT* pDtStructure, char* strTable);
void DTSTRUCT_AddField  (DTSTRUCT* pDtStructure, FIELD* pField);
void DTSTRUCT_SetFieldTerminator  (DTSTRUCT* pDtStructure, char* strTerminator);
void DTSTRUCT_SetFieldQuoteChar  (DTSTRUCT* pDtStructure, char *strQuoteChar); /* address of single or double byte char */
void DTSTRUCT_SetNullSubstChar   (DTSTRUCT* pDtStructure, char *strSubstChar); /* address of single byte char */
int  DTSTRUCT_SetFixLength(DTSTRUCT* pDtStructure, int nFixLength);
void DTSTRUCT_SetDateFormat(DTSTRUCT* pDtStructure, char* pDateFormat);
void DTSTRUCT_SetKeepTrailingBlanks (DTSTRUCT* pDtStructure); /* to be called if trailing blanks to be kept for all columns */

char* DTSTRUCT_GetInfile (DTSTRUCT* pDtStructure, int num);
char* DTSTRUCT_GetBadfile(DTSTRUCT* pDtStructure, int num);
char* DTSTRUCT_GetDiscardFile(DTSTRUCT* pDtStructure, int num);
i4  DTSTRUCT_GetDiscardMax(DTSTRUCT* pDtStructure);
char* DTSTRUCT_GetTable  (DTSTRUCT* pDtStructure);
char* DTSTRUCT_GetFieldTerminator  (DTSTRUCT* pDtStructure);
char* DTSTRUCT_GetFieldQuoteChar   (DTSTRUCT* pDtStructure);
char* DTSTRUCT_GetNullSubstChar    (DTSTRUCT* pDtStructure, int num);
int   DTSTRUCT_ColumnLevelFieldTerminator(DTSTRUCT* pDtStructure);
int   DTSTRUCT_GetFixLength(DTSTRUCT* pDtStructure, int num);
char* DTSTRUCT_GetDateFormat(DTSTRUCT* pDtStructure);
int   DTSTRUCT_GetKeepTrailingBlanks (DTSTRUCT* pDtStructure);
void LISTCHAR_pRECORD_Free(LISTCHAR* pRecord);



typedef struct tagCOMMANDLINE
{
	char szVnode[1024];
	char szControlFile[1024];
	char szLogFile[1024];
	char szUFlag[256];
	int  nParallel;
	int  nDirect;
	i4 lReadSize;
} COMMANDLINE;

extern COMMANDLINE g_cmd;
extern FILE* g_fLogFile;
void CMD_SetControlFile(COMMANDLINE* pCmdStructure, char* strFile);
void CMD_SetLogFile(COMMANDLINE* pCmdStructure, char* strFile);
void CMD_SetParallel(COMMANDLINE* pCmdStructure, int nSet);
void CMD_SetDirect(COMMANDLINE* pCmdStructure, int nSet);
void CMD_SetReadSize(COMMANDLINE* pCmdStructure, i4 lReadSize);
void CMD_SetVnode (COMMANDLINE* pCmdStructure, char* vNode);
void CMD_SetUFlag (COMMANDLINE* pCmdStructure, char* uFlag);

char* CMD_GetControlFile(COMMANDLINE* pCmdStructure);
char* CMD_GetLogFile(COMMANDLINE* pCmdStructure);
int   CMD_GetParallel(COMMANDLINE* pCmdStructure);
int   CMD_GetDirect(COMMANDLINE* pCmdStructure);
i4  CMD_GetReadSize(COMMANDLINE* pCmdStructure);
char* CMD_GetVnode (COMMANDLINE* pCmdStructure);
char* CMD_GetUFlag (COMMANDLINE* pCmdStructure);

typedef struct tagLOADDATA
{
	int nEnd;                /* 1: The reader thread has finished reading from the input file */
	LISTCHAR* pHead;         /* Head of the list, (the reader fetchs from this point */
	LISTCHAR* pTail;         /* Tail of the list, (the writer, ie reader of input file write at this point */

} LOADDATA;

extern LOADDATA* g_pLoadData;
/*
** LOADDATA_AddRecord:
** strRecord must be the already allocated buffer.
*/
void LOADDATA_AddRecord(LOADDATA* pLoadData, char* strRecord);
int  LOADDATA_IsFinished(LOADDATA* pLoadData);
void LOADDATA_Finish(LOADDATA* pLoadData);
void LOADDATA_Free(LOADDATA* pLoadData);



/*
** Start the Thread that read the input data file
** and puts the data in the FIFO queue:
*/
void StartInputReaderThread(DTSTRUCT* pDataStructure);

/*
** Enter a loop reading the data
** from the FIFO queue, transfer the data to the DBMS by
** using the COPY INTO callback program:
** Return:
**   :  success
**   :  0 (sql error)
**   : -1 (fail)
*/

int StartTransferDataThread(DTSTRUCT* pDataStructure);
/*
** Write string to log file:
*/
void Write2Log (FILE* f, char* strLine);

#define WRITE2LOG0(sz) Write2Log (g_fLogFile, sz)
#define WRITE2LOG1(sz, p1) \
{\
	char buff[4000];\
	STprintf (buff, sz, p1);\
	Write2Log(g_fLogFile, buff);\
}\

#define WRITE2LOG2(sz, p1, p2) \
{\
	char buff[4000];\
	STprintf (buff, sz, p1, p2);\
	Write2Log(g_fLogFile, buff);\
}\

#define WRITE2LOG3(sz, p1, p2, p3) \
{\
	char buff[4000];\
	STprintf (buff, sz, p1, p2, p3);\
	Write2Log(g_fLogFile, buff);\
}\


#define E_WRITE2LOG0(sz) \
{\
	Write2Log (g_fLogFile, sz);\
	SIprintf(sz);\
}\

#define E_WRITE2LOG1(sz, p1) \
{\
	char buff[4000];\
	STprintf (buff, sz, p1);\
	Write2Log(g_fLogFile, buff);\
	SIprintf(buff);\
}\

#define E_WRITE2LOG2(sz, p1, p2) \
{\
	char buff[4000];\
	STprintf (buff, sz, p1, p2);\
	Write2Log(g_fLogFile, buff);\
	SIprintf(buff);\
}\

#define E_WRITE2LOG3(sz, p1, p2, p3) \
{\
	char buff[4000];\
	STprintf (buff, sz, p1, p2, p3);\
	Write2Log(g_fLogFile, buff);\
	SIprintf(buff);\
}\

#endif /* MAIN_DATALOADER_STRUCTURE_HEADER */
