/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
NEEDLIBS = DATALDRLIB LIBINGRES
PROGRAM = dataldr
*/

/*
**    Source   : dtloader.c : Defines the entry point for the console application.
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main structure for data loader
**
** History:
**
** xx-Jan-2004 (uk$so01)
**    Created.
** 01-Mar-2004 (uk$so1)
**    BUG #11187, In addtion to the log file, syntax errors detected 
**    in the control file in the data loader should also be reported in the stdout.
** 03-Mar-2004 (uk$so01)
**    BUG #111895 / Issue 13265678.
**    In data loader, if user specifies the control file without .ctl, 
**    then the program reports an error saying "Failed to create log file."
** 11-Mar-2004 (uk$so01)
**    BUG #111933 / ISSUE 13279711:
**    The data loader should not accept '\n' as a field separator, since this character is reserved 
**    for indicating the end of the record (at least when not in the fixed widths format)
** 07-Apr-2004 (uk$so01)
**    BUG #112107 / ISSUE 13328333 (Add .log only if the SPECIFIED log='file' where file has no extension)
** 13-May-2004 (uk$so01)
**    BUG #112305, Replace the use of STindex by STrindex and x_strstr
** 11-Jun-2004 (uk$so01)
**    SIR #111688, additional change to support multiple INFILEs
** 15-Dec-2004 (toumi01)
**    Port to Linux.
** 10-Mar-2005 (toumi01)
**    Jam hints for Linux build.
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
#include "tsynchro.h"
#include "parse.h"
#include "sqlcopy.h"
#if defined (NT_GENERIC)
#include <process.h>
#include <conio.h>
#endif
#define g_sVersion "1.0"
#if defined (LINUX)
#include <unistd.h>
#endif

DTLMUTEX      g_mutexLoadData;
DTLMUTEX      g_mutexWrite2Log;
READSIZEEVENT g_ReadSizeEvent;
DTSTRUCT      g_dataLoaderStruct;
COMMANDLINE   g_cmd;
LOADDATA*     g_pLoadData = NULL;
FILE* g_fLogFile = NULL;
int  g_lReadSize = READSIZE_DEF;
int  g_lRecordRead = 0;
int   g_nGetRecordLineIvoked = 0;
int  g_lCpuStarted = 0;
int  g_lCpuEnded = 0;

static void CleanVariables()
{
	DTSTRUCT_Free(&g_dataLoaderStruct);
	if (g_pLoadData)
	{
		LOADDATA_Free(g_pLoadData);
		free (g_pLoadData);
		g_pLoadData = NULL;
	}
	CleanDTLMutex(&g_mutexLoadData);
	CleanDTLMutex(&g_mutexWrite2Log);
	CleanReadsizeEvent(&g_ReadSizeEvent);
	if (g_fLogFile)
	{
		SIclose(g_fLogFile);
		g_fLogFile = NULL;
	}
	INGRESII_llCleanSql();
}

void Time2Chars (double dSecond, char* buffer)
{
	char* p;
	char buff1[16];
	char buff2[16];
	int m = 0;
	int h = 0;
	while (dSecond >= 60.0)
	{
		dSecond -= 60.0;
		m++;
	}
	while (m >= 60)
	{
		m -= 60;
		h++;
	}
	dSecond = dSecond*100;
	STprintf(buff1, "%04d", (int)dSecond);
	STlcopy (buff1, buff2, 2);
	p = buff1;
	p += 2;
	STprintf (buffer, "%02d%:%02d:%s", h, m, buff2);

	STcat(buffer, ".");
	STcat(buffer, p);
}

int 
main(int argc, char* argv[])
{
	time_t ltime;
	time_t tm_0, tm_1;
	char szIIdate[64];
	char szCommandLineString[1024];
	int i;
	double dtime;

	time( &tm_0 );

	g_pLoadData = (LOADDATA*)malloc (sizeof(LOADDATA));
	memset (g_pLoadData, 0, sizeof(LOADDATA));
	memset (&g_ReadSizeEvent, 0, sizeof(g_ReadSizeEvent));
	memset (&g_dataLoaderStruct, 0, sizeof (g_dataLoaderStruct));
	g_dataLoaderStruct.nArraySize = 5;
	g_dataLoaderStruct.arrayFiles = (INFILES*)malloc (g_dataLoaderStruct.nArraySize * sizeof(INFILES));
	memset (&g_mutexWrite2Log, 0, sizeof (g_mutexWrite2Log));
	szIIdate[0] = '\0';
	if (!IsInputDateCompatible(szIIdate))
	{
		if (szIIdate[0])
			DTSTRUCT_SetDateFormat(&g_dataLoaderStruct, szIIdate);
	}
	CreateDTLMutex(&g_mutexWrite2Log);

	/*
	** Construct the command line string from the argv[]
	*/
	szCommandLineString[0] = '\0';
	for (i=1; i<argc; i++)
	{
		if (i>1)
			STcat (szCommandLineString, " ");
		STcat (szCommandLineString, argv[i]);
	}
	/*
	** Parse the command line string and update the 
	** Data Loader structure:
	*/
	memset (&g_cmd, 0, sizeof (g_cmd));
	if (szCommandLineString[0])
	{
		STATUS st;
		LOCATION lf;
		char* strFle = NULL;
		int nOk = ParseCommandLine (szCommandLineString, &g_cmd);
		if (nOk == 0)
		{
			free (g_pLoadData);
			/*
			** Command line syntax error:
			*/
			SIprintf("Command line syntax error !!!\n");
			return 1;
		}
		else
		{
			/*
			** Check the MANDATORY command line:
			*/
			char* strItem = CMD_GetVnode(&g_cmd);
			if (strItem && !strItem[0])
			{
				free (g_pLoadData);
				SIprintf("Command line option missing: the connection string <vnode> is mandatory.\n");
				return 1;
			}
			strItem = CMD_GetControlFile(&g_cmd);
			if (strItem && !strItem[0])
			{
				free (g_pLoadData);
				SIprintf("Command line option missing: the control file is mandatory.\n");
				return 1;
			}
		}
		if (CMD_GetReadSize(&g_cmd) > 0)
			g_lReadSize = CMD_GetReadSize(&g_cmd);
		/*
		** Create the log file if needed:
		*/
		strFle = CMD_GetLogFile(&g_cmd);
		if (!strFle || !strFle[0])
		{
			char szBuff[1024];
			char* pFound;
			STcopy (CMD_GetControlFile(&g_cmd), szBuff);
			pFound = STrindex (szBuff, ".ctl", -1);
			if (pFound && (STlength (pFound) == 4) && STcompare(pFound, ".ctl") == 0)
			{
				pFound[0] = '\0';
				STcat (szBuff, ".log");
				CMD_SetLogFile(&g_cmd, szBuff);
			}
		}
		else
		{
			char szBuff[1024];
			char* pFound;
			STcopy (CMD_GetLogFile(&g_cmd), szBuff);
			pFound = STrindex (szBuff, ".", -1);
			if (!pFound) /* if logfile has no extension then add a .log extension */
			{
				STcopy (strFle, szBuff);
				STcat  (szBuff, ".log");
				CMD_SetLogFile(&g_cmd, szBuff);
			}
		}

		LOfroms (PATH&FILENAME, strFle, &lf);
		st = SIfopen (&lf, "w", SI_TXT, DT_SI_MAX_TXT_REC, &g_fLogFile);
		if (st != OK)
		{
			g_fLogFile = NULL;
			SIprintf ("Failed to create log file\n");
		}

		time( &ltime );
		WRITE2LOG3("%s: Release %s - Production on %s\n\n", INGRES_DATA_LOADER, g_sVersion, ctime(&ltime));
		WRITE2LOG0("Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.\n\n");
		WRITE2LOG1("Control File:\t%s\n", CMD_GetControlFile(&g_cmd));
		if (!g_mutexWrite2Log.nCreated)
		{
			E_WRITE2LOG1("%s: Failed to create the Mutex 'g_mutexWrite2Log' for writing log file\n", INGRES_DATA_LOADER);
			CleanVariables();
			return 1;
		}
		nOk = ParseControlFile(&g_dataLoaderStruct, &g_cmd);
		if (nOk == 0)
		{
			E_WRITE2LOG1("%s: Syntax error in the control file.\n", INGRES_DATA_LOADER);
			CleanVariables();
			return 1;
		}
		else
		if (nOk == -1)
		{
			E_WRITE2LOG1("%s: Cannot open control file.\n", INGRES_DATA_LOADER);
			CleanVariables();
			return 1;
		}
		else
		{
			int i;
			int nErrorAccesInfile=0;
			int nAllwithPosition=1;
			int nWithPosition = 0;
			int nPosError = 0;
			int nMax=0;
			int nWrongSep = 0;
			FIELDSTRUCT* pField = &(g_dataLoaderStruct.fields);
			FIELD* listField = pField->listField;
			int* aSet = GetControlFileMandatoryOptionSet();
			if (aSet[0] == 0)
			{
				E_WRITE2LOG1("%s: LOAD DATA is mandatory at the begining of the control file.\n", INGRES_DATA_LOADER);
				CleanVariables();
				return 1;
			}

			if (aSet[1] == 0 || !DTSTRUCT_GetInfile(&g_dataLoaderStruct, 0))
			{
				E_WRITE2LOG1("%s: INFILE is mandatory in the control file.\n", INGRES_DATA_LOADER);
				CleanVariables();
				return 1;
			}
			for (i=0; i<g_dataLoaderStruct.nFileCount; i++)
			{
				strFle = DTSTRUCT_GetInfile(&g_dataLoaderStruct, i);
				if (access(strFle, 0) == -1)
				{
					nErrorAccesInfile = 1;
					E_WRITE2LOG2("%s: INFILE '%s' cannot be accessed.\n", INGRES_DATA_LOADER, strFle);
				}
			}
			if (nErrorAccesInfile == 1)
			{
				CleanVariables();
				return 1;
			}

			if (aSet[5] == 0 || STlength (DTSTRUCT_GetTable(&g_dataLoaderStruct)) == 0)
			{
				E_WRITE2LOG1("%s: INTO TABLE <table>, a table name is mandatory in the control file.\n", INGRES_DATA_LOADER);
				CleanVariables();
				return 1;
			}

			if (aSet[6] == 0)
			{
				E_WRITE2LOG1("%s: APPEND key word is mandatory in the control file.\n", INGRES_DATA_LOADER);
				CleanVariables();
				return 1;
			}

			if (aSet[8] == 0)
			{
				E_WRITE2LOG1("%s: The table columns must be specified in the control file.\n", INGRES_DATA_LOADER);
				CleanVariables();
				return 1;
			}
			if (g_dataLoaderStruct.fields.szTerminator[0] && g_dataLoaderStruct.fields.szTerminator[0] == '\n')
				nWrongSep = 1;
			if (listField)
			{
				int p1 = 0, p2 = 0;
				while (listField)
				{
					if (listField->szTerminator[0] && strcmp (listField->szTerminator, "\\n") == 0)
						nWrongSep = 1;
					if (listField->pos1 <= p2)
						nPosError = 1;
					p1 = listField->pos1;
					p2 = listField->pos2;
					if (listField->pos1 <= 0 || listField->pos2 <= 0)
						nPosError = 1;
					if (listField->pos1 > listField->pos2 )
						nPosError = 1;
					if (listField->pos2 > nMax)
						nMax = listField->pos2;
					if (listField->pos1 == 0 && listField->pos2 == 0)
					{
						listField = listField->next;
						nAllwithPosition = 0;
						continue;
					}

					nWithPosition = 1;
					listField = listField->next;
				}
			}
			if (nWrongSep == 1)
			{
				E_WRITE2LOG1("%s: You should not use the '\\n' as field separator.\n", INGRES_DATA_LOADER);
				CleanVariables();
				return 1;
			}
			for (i=0; i<g_dataLoaderStruct.nFileCount; i++)
			{
				int j;
				if ((i>0) && (DTSTRUCT_GetFixLength(&g_dataLoaderStruct, 0) != DTSTRUCT_GetFixLength(&g_dataLoaderStruct, i)))
				{
					E_WRITE2LOG1("%s: FIX n, fixed record length must be the same for all INFILE clauses.\n", INGRES_DATA_LOADER);
					CleanVariables();
					return 1;
				}
				/*
				** Check if there are multiple bad files with the same name:
				*/
				for (j=i+1; j<g_dataLoaderStruct.nFileCount;j++)
				{
					if (stricmp (g_dataLoaderStruct.arrayFiles[i].badfile, g_dataLoaderStruct.arrayFiles[j].badfile)==0)
					{
						E_WRITE2LOG1("%s: Multiple bad files with the same name. You should specify different BADFILE for each INFILE.\n", INGRES_DATA_LOADER);
						CleanVariables();
						return 1;
					}
				}
			}
			if (DTSTRUCT_GetFixLength(&g_dataLoaderStruct, 0) > 0)
			{
				if (nWithPosition && !nAllwithPosition)
				{
					E_WRITE2LOG1("%s: FIX n, fixed record length is specified, if you use POSITION (n, p) then all columns must use the POSITION.\n", INGRES_DATA_LOADER);
					CleanVariables();
					return 1;
				}
				if (nPosError == 1)
				{
					E_WRITE2LOG1("%s: If you use POSITION (n, p) then n, p should be > 0.\n", INGRES_DATA_LOADER);
					CleanVariables();
					return 1;
				}

				if (nMax > DTSTRUCT_GetFixLength(&g_dataLoaderStruct, 0))
				{
					E_WRITE2LOG1("%s: In the last POSITION (n, p), p should be <= FIX value.\n", INGRES_DATA_LOADER);
					CleanVariables();
					return 1;
				}
			}
			else
			{
				if (nWithPosition == 1)
				{
					E_WRITE2LOG1("%s: If you use POSITION (n, p) then you should also specify FIX value.\n", INGRES_DATA_LOADER);
					CleanVariables();
					return 1;
				}
			}

			if (FIELDSTRUCT_GetColumnCount (pField) <= 0)
			{
				E_WRITE2LOG1("%s: The list of columns should be specified.\n", INGRES_DATA_LOADER);
				CleanVariables();
				return 1;
			}
		}

		for (i=0; i<g_dataLoaderStruct.nFileCount; i++)
		{
			WRITE2LOG1("Data File:\t%s\n", DTSTRUCT_GetInfile(&g_dataLoaderStruct, i));
			if (DTSTRUCT_GetFixLength(&g_dataLoaderStruct, i) > 0)
				WRITE2LOG1("  File processing option string: \"fix %d\"\n", DTSTRUCT_GetFixLength(&g_dataLoaderStruct, i));
			WRITE2LOG1("  Bad File:\t%s\n", DTSTRUCT_GetBadfile(&g_dataLoaderStruct, i));
			if (DTSTRUCT_GetDiscardFile(&g_dataLoaderStruct, i))
				WRITE2LOG1("  Discard File:\t%s\n", DTSTRUCT_GetDiscardFile(&g_dataLoaderStruct, i));
			}
		WRITE2LOG1(" (Allow %d discards)\n", DTSTRUCT_GetDiscardMax(&g_dataLoaderStruct));

		WRITE2LOG0("\n\n");
		WRITE2LOG1("Table %s\n", DTSTRUCT_GetTable(&g_dataLoaderStruct));
		WRITE2LOG0("Insert option in effect for this table: APPEND\n");
		
		/*
		WRITE2LOG1("Load to Table:\t%s\n", DTSTRUCT_GetTable(&g_dataLoaderStruct));
		WRITE2LOG1("Read Size:\t%d\n", g_lReadSize);
		WRITE2LOG1("Parallel:\t%c\n", (CMD_GetParallel(&g_cmd) == 1)? 'Y': 'N');
		*/
		if (!CheckForKnownDateFormat())
			E_WRITE2LOG1("%s: The setting value of II_DATE_FORMAT is unknown.\n", INGRES_DATA_LOADER);

		memset (&g_mutexLoadData, 0, sizeof (DTLMUTEX));
		if (!CreateDTLMutex(&g_mutexLoadData))
		{
			E_WRITE2LOG1("%s: Failed to create the Mutex 'g_mutexLoadData'\n", INGRES_DATA_LOADER);
			CleanVariables();
			return 1;
		}
		if (!CreateReadsizeEvent(&g_ReadSizeEvent))
		{
			E_WRITE2LOG1 ("%s: Failed to create the Readsize Condition 'g_ReadSizeEvent'\n", INGRES_DATA_LOADER);
			CleanVariables();
			return 1;
		}

		/*
		** Create the session:
		*/
		nOk = INGRESII_llConnectSession(CMD_GetVnode(&g_cmd));
		if (nOk == 0) /* SQL Error */
		{
			char* strError = INGRESII_llGetLastSQLError();
			if (strError && strError[0])
				/* Error while connecting to the DBMS:*/
				WRITE2LOG0(strError);
			CleanVariables();
			return 1;
		}

		for (i=0; i<g_dataLoaderStruct.nFileCount; i++)
		{
			/*
			** Start the thread that reads the input data file and
			** puts the data in the FIFO queue:
			*/
			StartInputReaderThread(&g_dataLoaderStruct);

			/*
			** The main thread begins a loop reading the data
			** from the FIFO queue, transfer the data to the DBMS by
			** using the COPY INTO callback program:
			*/
			nOk = StartTransferDataThread(&g_dataLoaderStruct);
			if (nOk == 0) /* SQL Error */
			{
				char* strError = INGRESII_llGetLastSQLError();
				if (strError && strError[0])
					WRITE2LOG0 (strError);
			}
			g_dataLoaderStruct.nCurrent++;
			memset (g_pLoadData, 0, sizeof(LOADDATA));
		}
		nOk = INGRESII_llDisconnectSession(1, 1);
		if (nOk == 0) /* SQL Error */
		{
			char* strError = INGRESII_llGetLastSQLError();
			if (strError && strError[0])
				WRITE2LOG0 (strError);
		}
	}
	else
	{
		SIprintf("Usage: \tdataldr [vnode::]dbname[/serverclass] control=controlfile\n");
		SIprintf("\t[log=logfile][parallel=y] [direct=y] [readsize=value]");
	}

	time( &tm_1 );
	dtime = difftime (tm_1, tm_0);

	WRITE2LOG0("\n\n");
	WRITE2LOG1("Run began on %s",   ctime(&tm_0));
	WRITE2LOG1("Run ended on %s\n", ctime(&tm_1));
	WRITE2LOG0("\n\n");

	Time2Chars(dtime, szIIdate);
	WRITE2LOG1("Elapsed time was:    %s\n", szIIdate);
	STcopy ("00:00:00", szIIdate);
	if ((g_lCpuEnded - g_lCpuStarted) >= 0)
	{
		double ds = 0.0, d1;
		int lms = g_lCpuEnded - g_lCpuStarted;
		while (lms >=1000)
		{
			lms -= 1000;
			ds += 1.0;
		}
		d1 = (double)lms / 10.0; /* 1/100 s == 10 ms */
		d1 = d1 / 100.0;         /* into the seconds */
		ds += d1;
		Time2Chars(ds, szIIdate);
	}
	WRITE2LOG1("CPU time was:        %s\n", szIIdate);

	/*
	** Free the structure before exiting:
	*/
	CleanVariables();
	return 0;
}

