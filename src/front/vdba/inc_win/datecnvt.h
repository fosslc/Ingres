/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : datecnvt.h : header file
**    Project  : INGRES II/Libingll (extend library)
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Date and date conversion.
**
** History:
**
** 29-Jan-2003 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
*/

typedef enum tagIIDateType
{
	II_DATE_UNKNOWN = 0,
	II_DATE_US,              // dd-mmm-yyyy
	II_DATE_MULTINATIONAL,   // dd/mm/yy
	II_DATE_MULTINATIONAL4,  // dd/mm/yyyy
	II_DATE_ISO,             // yymmdd
	II_DATE_ISO4,            // yyyymmdd
	II_DATE_SWEDENxFINLAND,  // yyyy-mm-dd
	II_DATE_GERMAN,          // dd.mm.yyyy
	II_DATE_YMD,             // yyyy-mmm-dd
	II_DATE_DMY              // dd-mmm-yyyy
} IIDateType, II_DATEFORMAT;

BOOL INGRESII_IIDateFromString(LPCTSTR lpszInputString, IIDateType& nOutputIIDateFormat);
BOOL INGRESII_ConvertDate (CString& strDate, IIDateType nInputFormat, IIDateType nOutputFormat, int nCentury = 0);


