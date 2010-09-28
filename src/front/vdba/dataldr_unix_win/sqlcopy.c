/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlcopy.c : Implementation file
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : SQL Session & SQL Copy, include the esqlc generated file
**
** History:
**
** 02-Jan-2004 (uk$so01)
**    Created.
** 23-Apr-2004 (uk$so01)
**    DATA LOADER BUG #112187 / ISSUE 13386574 
**    Data loader incorrectly loads data when using BLOB files.
** 11-Jun-2004 (uk$so01)
**    SIR #111688, additional change to support multiple INFILEs
** 12-May-2006 (thaju02)
**    In GenerateCopySyntax(), if LNOBFILE/COLACT_LNOB use 'long nvarchar'
**    for column's copy format. 
** 11-Aug-2010 (kschendel) b124231
**	Properly generate the trailing nl delimiter when there are
**	sequence columns.  Apparently the old COPY code somehow managed
**	to eat a trailing newline even when it's not being asked for,
**	but the new COPY code only does exactly what it's told.
*/

#include "stdafx.h"
#include "sqlcopy.h"
#include "dtstruct.h"
#include <nm.h>

#if defined (UNIX)
#define consttchszReturn   "\n\0"
#define consttchszPathSep  "/"
#else
#define consttchszReturn   "\r\n\0"
#define consttchszPathSep  "\\"
#endif

/*
** Include the esqlc generated file from "sqlcopy.sc":
*/
#include "sqlcopy.inc"

/*
** Use the copy into syntax with column format = varchar(0)
** stored as variable-length string preceded by 5-character, 
** right-justified length specifier:
*/

char* GenerateCopySyntax(DTSTRUCT* pDataStructure, int num)
{
	int  i, m;
	char szColName[68];
	char szReadFormat[32];
	char szBadFile[1050];
	char* strOut = (char*)0;
	FIELDSTRUCT* pField = &(pDataStructure->fields);
	FIELD* listField = pField->listField;
	int nCount = FIELDSTRUCT_GetColumnCount(pField);
	int nHasSequence = FIELDSTRUCT_GetSequenceCount(pField);
	int nOne = 1;

	STprintf (szBadFile, "'%s'", DTSTRUCT_GetBadfile(pDataStructure, num));
	m = ConcateStrings (
		&strOut, 
		"copy table ", 
		pDataStructure->szTableName,
		(char*)0);
	m = ConcateStrings (
		&strOut, 
		"(",
		consttchszReturn, 
		(char*)0);

	i=0;
	while (listField)
	{
		int nNullable = (listField->nColAction & COLACT_WITHNULL);
		if (listField->nFiller == 1)
		{
			/*
			** Do not generate the filler column in the copy into statement 
			** because we use the varchar(0):
			*/
			listField = listField->next;
			pField->pArrayPosition[i] = 2;
			i++;
			continue;
		}
		if (listField->nColAction & COLACT_SEQUENCE)
		{
			/*
			** The sequence columns are pushed to the end:
			*/
			listField = listField->next;
			i++;
			continue;
		}

		STcopy ("=varchar(0) ", szReadFormat);
		if (nOne == 0)
		{
			m = ConcateStrings (
				&strOut, 
				", ",
				consttchszReturn, 
				(char*)0);
			if (!m)
				return 0;
		}

		if (listField->nColAction & COLACT_LOBFILE)
		{
			pField->pArrayPosition[i] = 1;
			if (listField->nColAction & COLACT_LNOB)
			    STcopy ("=Long nvarchar(0) ", szReadFormat);
			else
			    STcopy ("=Long varchar(0) ", szReadFormat);
		}
		i++;

		STprintf (szColName, "\"%s\"", listField->szColumnName);
		m = ConcateStrings (
			&strOut, 
			szColName,                 /* column name */
			szReadFormat,              /* read format */
			(char*)0);
#if defined (_ADD_NL_)
		if (listField->next == NULL && nHasSequence == 0)
		{
			m = ConcateStrings (
				&strOut, 
				"nl ",
				(char*)0);
		}
#endif
		if (nNullable)
		{
			char buff [256];
			STprintf (buff, "with null ('%s')", (char*)listField->szNullValue);
			m = ConcateStrings (
				&strOut, 
				buff,                  /* with null */
				(char*)0);
		}

		nOne = 0;
		listField = listField->next;
	}

	/*
	** Put the sequence columns at the end:
	*/
	if (nHasSequence > 0)
	{
		listField = pField->listField;
		while (listField)
		{
			int nNullable = (listField->nColAction & COLACT_WITHNULL);;
			if (!(listField->nColAction & COLACT_SEQUENCE))
			{
				listField = listField->next;
				continue;
			}

			if (nOne == 0)
			{
				m = ConcateStrings (
					&strOut, 
					", ",
					consttchszReturn, 
					(char*)0);
				if (!m)
					return 0;
			}

			STprintf (szColName, "\"%s\"", listField->szColumnName);
			m = ConcateStrings (
				&strOut, 
				szColName,                 /* column name */
				"=varchar(0) ",            /* read format */
				(char*)0);
			nOne = 0;
			listField = listField->next;
		}
		m = ConcateStrings (
				&strOut, 
				"nl ",
				(char*)0);
	}

	m = ConcateStrings (
		&strOut, 
		") ",
		consttchszReturn,
		"from program with on_error=continue,log=", /*error_count=10000*/
		szBadFile,
		(char*)0);

	return strOut;
}

#define II_DATE_FORMAT_MAX 11
int CheckForKnownDateFormat()
{
	char szIIDate [32];
	char szKnownDateF[II_DATE_FORMAT_MAX][16] = 
	{
		"US",
		"MULTINATIONAL",
		"MULTINATIONAL4",
		"ISO",
		"ISO4",
		"SWEDEN",
		"FINLAND",
		"GERMAN",
		"YMD",
		"DMY",
		"MDY"
	};

	int nFound = 1; /* default is US */
	char* strIIdate = NULL;
	NMgtAt("II_DATE_FORMAT", &strIIdate);
	if (strIIdate && strIIdate[0])
	{
		int i;
		nFound = 0; /* If II_DATE_FORMAT is set, it should be one of the elements in the array */
		STcopy(strIIdate, szIIDate);
		for (i=0; i<II_DATE_FORMAT_MAX; i++)
		{
			if (stricmp (szIIDate, szKnownDateF[i]) == 0)
			{
				nFound = 1;
				break;
			}
		}
	}

	return nFound;
}


/*
** Up to Francois, the input date format is always 'mm/dd/yyyy hh24:mi:ss'
** I conclude that this input format will be accepted by almost ingres II_DATE_FORMAT setting
** except that YMD, DMY, MDY.
** Thus, this function checks if YMD, DMY or MDY is being used as II_DATE_FORMAT, if so
** the conversion is necessary.
** OUT: szCurrentIIdateFormat is the II_DATE_FORMAT value
** RETURN 0 if (szCurrentIIdateFormat is one of YMD, DMY or MDY)
*/
int IsInputDateCompatible(char* szCurrentIIdateFormat)
{
	char* strIIdate = NULL;
	NMgtAt("II_DATE_FORMAT", &strIIdate);
	if (strIIdate && strIIdate[0])
	{
		STcopy(strIIdate, szCurrentIIdateFormat);
		if (stricmp (szCurrentIIdateFormat, "YMD") == 0)
			return 0;
		else
		if (stricmp (szCurrentIIdateFormat, "DMY") == 0)
			return 0;
		else
		if (stricmp (szCurrentIIdateFormat, "MDY") == 0)
			return 0;
		return 1;
	}
	/*
	** default US format:
	*/
	return 1;
}

/*
** Convert date in szDateInput to szDateOutput according to the 'szIIDate':
*/
void INGRESII_DateConvert(char* szIIDate, char* szDateInput, char* szDateOutput)
{
	/*                 0123456789
	** INPUT format = 'mm/dd/yyyy hh24:mi:ss'
	*/
	STcopy (szDateInput, szDateOutput);
	if (STlength (szDateInput) < 9)
		return;
	if (stricmp (szIIDate, "YMD") == 0)
	{
		/*
		** Convert to yyyy-mm-dd:
		*/
		szDateOutput[0] = szDateInput[6];
		szDateOutput[1] = szDateInput[7];
		szDateOutput[2] = szDateInput[8];
		szDateOutput[3] = szDateInput[9];
		szDateOutput[4] = '-';
		szDateOutput[5] = szDateInput[0];
		szDateOutput[6] = szDateInput[1];
		szDateOutput[7] = '-';
		szDateOutput[8] = szDateInput[3];
		szDateOutput[9] = szDateInput[4];
	}
	else
	if (stricmp (szIIDate, "DMY") == 0 || stricmp (szIIDate, "MDY") == 0)
	{
		/*
		** Convert to dd-mm-yyyy:
		*/
		szDateOutput[0] = szDateInput[3];
		szDateOutput[1] = szDateInput[4];
		szDateOutput[2] = '-';
		szDateOutput[3] = szDateInput[0];
		szDateOutput[4] = szDateInput[1];
		szDateOutput[5] = '-';
	}
}

