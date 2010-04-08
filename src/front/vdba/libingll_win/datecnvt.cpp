/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : datecnvt.cpp : Implementation file
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

#include "stdafx.h"
#include "datecnvt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static int nUSMonth (LPCTSTR lpszMonth, int nPosStart);
static CString sUsMonth (int nMonth);


BOOL INGRESII_IIDateFromString(LPCTSTR lpszInputString, IIDateType& nOutputIIDateFormat)
{
	BOOL bOK = TRUE;
	CString strWhat = lpszInputString;
	nOutputIIDateFormat = II_DATE_UNKNOWN;
	strWhat.MakeUpper();

	if (strWhat == _T("US"))
		nOutputIIDateFormat = II_DATE_US;
	else
	if (strWhat == _T("MULTINATIONAL"))
		nOutputIIDateFormat = II_DATE_MULTINATIONAL;
	else
	if (strWhat == _T("MULTINATIONAL4"))
		nOutputIIDateFormat = II_DATE_MULTINATIONAL4;
	else
	if (strWhat == _T("ISO"))
		nOutputIIDateFormat = II_DATE_ISO;
	else
	if (strWhat == _T("ISO4"))
		nOutputIIDateFormat = II_DATE_ISO4;
	else
	if (strWhat == _T("SWEDEN") || strWhat == _T("FINLAND"))
		nOutputIIDateFormat = II_DATE_SWEDENxFINLAND;
	else
	if (strWhat == _T("GERMAN"))
		nOutputIIDateFormat = II_DATE_GERMAN;
	else
	if (strWhat == _T("YMD"))
		nOutputIIDateFormat = II_DATE_YMD;
	else
	if (strWhat == _T("DMY"))
		nOutputIIDateFormat = II_DATE_DMY;
	else
		bOK = FALSE;
	return bOK;
}

//
// CTime only accept the date in the interval
// [01-Jan-1970 at 00:00:00, 18-Jan-2038 at 19:14:07]
BOOL INGRESII_ConvertDate (CString& strDate, IIDateType nInputFormat, IIDateType nOutputFormat, int nCentury)
{
	BOOL bOK = TRUE;
	CString s = strDate; // dd-mmm-yyyy
	TCHAR tchszDay   [3];
	TCHAR tchszYear  [5];
	TCHAR tchszMonth [4];
	LPTSTR lpszYear2 = NULL;
	int nMonth = 0;
	CTime t;

	switch (nInputFormat)
	{
	case II_DATE_US:              // dd-mmm-yyyy
	case II_DATE_DMY:             // dd-mmm-yyyy
		if (strDate.GetLength() < 11)
			return FALSE;
		tchszDay[0] = s[0];
		tchszDay[1] = s[1];
		tchszDay[2] = _T('\0');
		nMonth = nUSMonth (s, 3);
		tchszYear[0] = s[7];
		tchszYear[1] = s[8];
		tchszYear[2] = s[9];
		tchszYear[3] = s[10];
		tchszYear[4] = _T('\0');
		break;
	case II_DATE_MULTINATIONAL:   // dd/mm/yy
		if (strDate.GetLength() < 8)
			return FALSE;
		tchszDay[0] = s[0];
		tchszDay[1] = s[1];
		tchszDay[2] = _T('\0');
		tchszMonth[0] = s[3];
		tchszMonth[1] = s[4];
		tchszMonth[2] = _T('\0');
		tchszYear[2] = s[6];
		tchszYear[3] = s[7];
		tchszYear[4] = _T('\0');
		nMonth = _ttoi (tchszMonth);
		break;
	case II_DATE_MULTINATIONAL4:  // dd/mm/yyyy
	case II_DATE_GERMAN:          // dd.mm.yyyy
		if (strDate.GetLength() < 10)
			return FALSE;
		tchszDay[0] = s[0];
		tchszDay[1] = s[1];
		tchszDay[2] = _T('\0');
		tchszMonth[0] = s[3];
		tchszMonth[1] = s[4];
		tchszMonth[2] = _T('\0');
		tchszYear[0] = s[6];
		tchszYear[1] = s[7];
		tchszYear[2] = s[8];
		tchszYear[3] = s[9];
		tchszYear[4] = _T('\0');
		nMonth = _ttoi (tchszMonth);
		break;
	case II_DATE_ISO:             // yymmdd
		if (strDate.GetLength() < 6)
			return FALSE;
		tchszDay[0] = s[4];
		tchszDay[1] = s[5];
		tchszDay[2] = _T('\0');
		tchszMonth[0] = s[2];
		tchszMonth[1] = s[3];
		tchszMonth[2] = _T('\0');
		tchszYear[2] = s[0];
		tchszYear[3] = s[1];
		tchszYear[4] = _T('\0');
		nMonth = _ttoi (tchszMonth);
		break;
	case II_DATE_ISO4:            // yyyymmdd
		if (strDate.GetLength() < 8)
			return FALSE;
		tchszDay[0] = s[6];
		tchszDay[1] = s[7];
		tchszDay[2] = _T('\0');
		tchszMonth[0] = s[4];
		tchszMonth[1] = s[5];
		tchszMonth[2] = _T('\0');
		tchszYear[0] = s[0];
		tchszYear[1] = s[1];
		tchszYear[2] = s[2];
		tchszYear[3] = s[3];
		tchszYear[4] = _T('\0');
		nMonth = _ttoi (tchszMonth);
		break;
	case II_DATE_SWEDENxFINLAND:  // yyyy-mm-dd
		if (strDate.GetLength() < 10)
			return FALSE;
		tchszDay[0] = s[8];
		tchszDay[1] = s[9];
		tchszDay[2] = _T('\0');
		tchszMonth[0] = s[5];
		tchszMonth[1] = s[6];
		tchszMonth[2] = _T('\0');
		tchszYear[0] = s[0];
		tchszYear[1] = s[1];
		tchszYear[2] = s[2];
		tchszYear[3] = s[3];
		tchszYear[4] = _T('\0');
		nMonth = _ttoi (tchszMonth);
		break;
	case II_DATE_YMD:             // yyyy-mmm-dd
		if (strDate.GetLength() < 11)
			return FALSE;
		tchszDay[0] = s[9];
		tchszDay[1] = s[10];
		tchszDay[2] = _T('\0');
		nMonth = nUSMonth (s, 5);
		tchszYear[0] = s[0];
		tchszYear[1] = s[1];
		tchszYear[2] = s[2];
		tchszYear[3] = s[3];
		tchszYear[4] = _T('\0');
		break;
	default:
		bOK = FALSE;
		ASSERT(FALSE);
		return bOK;
	}
	
	if (nCentury > 0 && nCentury < 100)
	{
		lpszYear2 = &tchszYear[2];
		if (_ttoi(lpszYear2) <= nCentury)
		{
			tchszYear[0] = _T('2');
			tchszYear[1] = _T('0');
		}
		else
		{
			tchszYear[0] = _T('1');
			tchszYear[1] = _T('9');
		}
	}
	else // Current century
	{
		tchszYear[0] = _T('2');
		tchszYear[1] = _T('0');
	}

	//
	// Do not use CTime because it accepts only date between [01-Jan-1970 at 00:00:00, 18-Jan-2038 at 19:14:07]
	// t = CTime(_ttoi (tchszYear), nMonth, _ttoi (tchszDay), 0, 0, 0, 0);
	bOK = TRUE;
	switch (nOutputFormat)
	{
	case II_DATE_US:              // dd-mmm-yyyy
		strDate.Format(_T("%s-%s-%s"), tchszDay, (LPCTSTR)sUsMonth(nMonth), tchszYear);
		break;
	case II_DATE_MULTINATIONAL:   // dd/mm/yy
		lpszYear2 = &tchszYear[2];
		strDate.Format(_T("%s/%02d/%s"), tchszDay, nMonth, lpszYear2);
		break;
	case II_DATE_MULTINATIONAL4:  // dd/mm/yyyy
		strDate.Format(_T("%s/%02d/%s"), tchszDay, nMonth, tchszYear);
		break;
	case II_DATE_ISO:             // yymmdd
		lpszYear2 = &tchszYear[2];
		strDate.Format(_T("%s%02d%s"), lpszYear2, nMonth, tchszDay);
		break;
	case II_DATE_ISO4:            // yyyymmdd
		strDate.Format(_T("%s%02d%s"), tchszYear, nMonth, tchszDay);
		break;
	case II_DATE_SWEDENxFINLAND:  // yyyy-mm-dd
		strDate.Format (_T("%s-%02d-%s"), tchszYear, nMonth, tchszDay); 
		break;
	case II_DATE_GERMAN:          // dd.mm.yyyy
		strDate.Format (_T("%s.%02d.%s"), tchszDay, nMonth, tchszYear); 
		break;
	case II_DATE_YMD:             // yyyy-mmm-dd
		strDate.Format (_T("%s-%s-%s"), tchszYear, (LPCTSTR)sUsMonth(nMonth), tchszDay); 
		break;
	case II_DATE_DMY:             // dd-mmm-yyyy
		strDate.Format (_T("%s-%s-%s"), tchszDay, (LPCTSTR)sUsMonth(nMonth), tchszYear); 
		break;
	default:
		bOK = FALSE;
		ASSERT(FALSE);
		break;
	}

	return bOK;
}

static int nUSMonth (LPCTSTR lpszMonth, int nPosStart)
{
	TCHAR tchszMonth [12][4] = 
	{
		_T("Jan"),
		_T("Feb"),
		_T("Mar"),
		_T("Apr"),
		_T("May"),
		_T("Jun"),
		_T("Jul"),
		_T("Aug"),
		_T("Sep"),
		_T("Oct"),
		_T("Nov"),
		_T("Dec"),
	};

	CString strMonth = lpszMonth;
	CString s = strMonth.Mid(nPosStart, 3);
	for (int i=0; i<12; i++)
	{
		if (s.CompareNoCase (tchszMonth[i]) == 0)
			return i+1;
	}
	//
	// Default set to January !
	ASSERT (FALSE);
	return 1;
}

static CString sUsMonth (int nMonth)
{
	TCHAR tchszMonth [12][4] = 
	{
		_T("Jan"),
		_T("Feb"),
		_T("Mar"),
		_T("Apr"),
		_T("May"),
		_T("Jun"),
		_T("Jul"),
		_T("Aug"),
		_T("Sep"),
		_T("Oct"),
		_T("Nov"),
		_T("Dec"),
	};

	for (int i=0; i<12; i++)
	{
		if (nMonth == (i+1))
			return tchszMonth[i];
	}
	//
	// Default set to January !
	ASSERT (FALSE);
	return tchszMonth[0];
}

