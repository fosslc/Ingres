/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Project  : VDBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : class CaCursor (using cursor to select data from table)
**
** History:
**
** xx-Jan-1998 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Enhance library (select loop)
** 29-Apr-2003 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM 
**    Handle new Ingres Version 2.65
**/


#include "stdafx.h"
#include "sqlselec.h"
#include "lsselres.h"
#include "sessimgr.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// This file "" is generated from ESQLCC -multi -fsqlselec.inc sqlselec.scc
#include "sqlselec.inc"


int CALLBACK SelectResultCompareSubItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nC, iV1, iV2;
	LPCTSTR  lpItem1 = (LPCTSTR)lParam1;
	LPCTSTR  lpItem2 = (LPCTSTR)lParam2;
	SORTROW* lpSortData = (SORTROW*)lParamSort;
	BOOL bAsc = lpSortData? lpSortData->bAsc: TRUE;
	int  nType= lpSortData? lpSortData->nDisplayType: SQLACT_TEXT;
	double d1, d2;

	if (lpItem1 && lpItem2)
	{
		switch (nType)
		{
		case SQLACT_TEXT:
		case SQLACT_DATE:
			return bAsc? lstrcmpi (lpItem1, lpItem2): (-1)*lstrcmpi (lpItem1, lpItem2);
		case SQLACT_NUMBER:
			iV1 = _ttoi (lpItem1);
			iV2 = _ttoi (lpItem2);
			nC  = (iV1 > iV2)? 1: (iV1 < iV2)? -1: 0;
			return bAsc? nC: (-1)*nC;
		case SQLACT_NUMBER_FLOAT:
			d1 = atof (lpItem1);
			d2 = atof (lpItem2);
			nC  = (d1 > d2)? 1: (d1 < d2)? -1: 0;
			return bAsc? nC: (-1)*nC;
		default:
			return bAsc? lstrcmpi (lpItem1, lpItem2): (-1)*lstrcmpi (lpItem1, lpItem2);
		}
	}
	else
	{
		nC = lpItem1? 1: lpItem2? -1: 0;
		return bAsc? nC: (-1)*nC;
	}
}

//
// Object: CaCursorInfo
// ************************************************************************************************
IMPLEMENT_SERIAL (CaCursorInfo, CaConnectInfo, 1)
CaCursorInfo::CaCursorInfo():CaConnectInfo()
{
	m_nFloat4Width     = 11;
	m_nFloat4Decimal   = 3; 
	m_ntchDisplayMode4 = _T('n');

	m_nFloat8Width     = 11;
	m_nFloat8Decimal   = 3;
	m_ntchDisplayMode8 = _T('n');
}

CaCursorInfo::CaCursorInfo(const CaCursorInfo& c)
{
	Copy(c);
}

CaCursorInfo CaCursorInfo::operator = (const CaCursorInfo& c)
{
	Copy (c);
	return *this;
}

void CaCursorInfo::Copy (const CaCursorInfo& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaConnectInfo::Copy(c);

	m_nFloat4Width  = c.m_nFloat4Width;
	m_nFloat4Decimal = c.m_nFloat4Decimal;
	m_ntchDisplayMode4 = c.m_ntchDisplayMode4;
	m_nFloat8Width = c.m_nFloat8Width;
	m_nFloat8Decimal = c.m_nFloat8Decimal;
	m_ntchDisplayMode8 = c.m_ntchDisplayMode8;
}

void CaCursorInfo::Serialize (CArchive& ar)
{
	CaConnectInfo::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_nFloat4Width;
		ar << m_nFloat4Decimal;
		ar << m_ntchDisplayMode4;
		ar << m_nFloat8Width;
		ar << m_nFloat8Decimal;
		ar << m_ntchDisplayMode8;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_nFloat4Width;
		ar >> m_nFloat4Decimal;
		ar >> m_ntchDisplayMode4;
		ar >> m_nFloat8Width;
		ar >> m_nFloat8Decimal;
		ar >> m_ntchDisplayMode8;
	}
}

//
// Object: CaFloatFormat
// ************************************************************************************************
IMPLEMENT_SERIAL (CaFloatFormat, CaCursorInfo, 1)


//
// Object: CaMoneyFormat
// ************************************************************************************************
CaMoneyFormat::CaMoneyFormat()
{
	//
	// Default format:
	lstrcpy (m_tchIIMoneyFormat, _T("$"));
	m_nIIMoneyPrec = 2;
	m_nIILeadOrTrail = MONEY_LEAD;
	m_bFormatMoney = TRUE;
	Initialize();
}


BOOL CaMoneyFormat::Initialize()
{
	USES_CONVERSION;
	BOOL bOK = TRUE;
	char* moneybuf = NULL; // Must be ansi string to be used be campat library function
	LPTSTR lpDupString = NULL, temp = NULL;
	NMgtAt("II_MONEY_FORMAT", &moneybuf);
	if ( moneybuf != NULL && *moneybuf != EOS ) // II_MONEY_FORMAT not set
	{
		lpDupString = _tcsdup(A2T(moneybuf));
		temp = lpDupString;
		temp = _tcsinc(temp);
		if ((*moneybuf == 'n' || *moneybuf == 'N') && strlen(moneybuf) == 4)
		{
			if (_tcsicmp(lpDupString, _T("none")) == 0)
				*m_tchIIMoneyFormat = EOS;
			else
			{
				bOK = FALSE;
			}
		}
		else 
		if ((*moneybuf != 'l' && *moneybuf != 'L' &&
		     *moneybuf != 't' && *moneybuf != 'T' ) ||
		     *temp     != ':' /* {l|t}:$$$ */ ||
		     strlen(moneybuf) > MAX_MONEY_FORMAT + 2 || strlen(moneybuf) < 3)
		{
			bOK = FALSE;
			//
			// default is "l:$"; leading currency symbol of "$" 
		}
		else
		{
			if ( *moneybuf == 'l' || *moneybuf == 'L')
			{
				m_nIILeadOrTrail = LEAD_MONEY;
			}
			else if ( *moneybuf == 't' || *moneybuf == 'T')
			{
				m_nIILeadOrTrail = TRAIL_MONEY;
			}
			temp = _tcsinc(temp);// skip the colon character ':'
			_tcscpy(m_tchIIMoneyFormat, temp);
		}
	}

	if (lpDupString)
		free (lpDupString);

	NMgtAt("II_MONEY_PREC", &moneybuf);
	m_nIIMoneyPrec = 2;
	if ( moneybuf != NULL && *moneybuf != EOS )
	{
		if (*moneybuf == '0')
			m_nIIMoneyPrec = 0;
		else if (*moneybuf == '1')
			m_nIIMoneyPrec = 1;

		else if (*moneybuf != '2')
		{
			bOK = FALSE;
		}
	}
	return bOK;
}


//
// Object: CaDecimalFormat
// ************************************************************************************************
CaDecimalFormat::CaDecimalFormat()
{
	//
	// Default value when II_DECIMAL is not defined:
	m_tchDecimalSeparator = _T('.');
	Initialize();
}

BOOL CaDecimalFormat::Initialize()
{
	USES_CONVERSION;
	BOOL bOK = TRUE;
	char* temp; // Must be ansi string to be used be campat library function

	NMgtAt("II_DECIMAL", &temp);
	if( temp != NULL && *temp != EOS )
	{
		if ( temp[0] != '.' && temp[0] != ',')
		{
			bOK = FALSE;
		}
		else
			m_tchDecimalSeparator = temp[0];
	}

	return bOK;
}



//
// Utility functions for float preferences:
// ************************************************************************************************
void F4_OnChangeEditPrecision(CEdit* pCtrl) 
{
	CString strText;
	if (!pCtrl)
		return;
	if (!pCtrl->m_hWnd)
		return;
	pCtrl->GetWindowText(strText);
	int iVal = (int)atoi ((LPCTSTR)strText);
	if (iVal < MINF4_PREC)
	{
		MessageBeep (-1);
		strText.Format (_T("%d"), MINF4_PREC);
		pCtrl->SetWindowText (strText);
	}
	else
	if (iVal > MAXF4_PREC)
	{
		MessageBeep (-1);
		strText.Format (_T("%d"), MAXF4_PREC);
		pCtrl->SetWindowText (strText);
	}
}


void F4_OnChangeEditScale(CEdit* pCtrl) 
{
	CString strText;
	if (!pCtrl)
		return;
	if (!pCtrl->m_hWnd)
		return;
	pCtrl->GetWindowText(strText);
	int iVal = (int)atoi ((LPCTSTR)strText);
	if (iVal < MINF4_SCALE)
	{
		MessageBeep (-1);
		strText.Format (_T("%d"), MINF4_SCALE);
		pCtrl->SetWindowText (strText);
	}
	else
	if (iVal > MAXF4_SCALE)
	{
		MessageBeep (-1);
		strText.Format (_T("%d"), MAXF4_SCALE);
		pCtrl->SetWindowText (strText);
	}
}

void F8_OnChangeEditPrecision(CEdit* pCtrl) 
{
	CString strText;
	if (!pCtrl)
		return;
	if (!pCtrl->m_hWnd)
		return;
	pCtrl->GetWindowText(strText);
	int iVal = (int)atoi ((LPCTSTR)strText);
	if (iVal < MINF8_PREC)
	{
		MessageBeep (-1);
		strText.Format (_T("%d"), MINF8_PREC);
		pCtrl->SetWindowText (strText);
	}
	else
	if (iVal > MAXF8_PREC)
	{
		MessageBeep (-1);
		strText.Format (_T("%d"), MAXF8_PREC);
		pCtrl->SetWindowText (strText);
	}
}


void F8_OnChangeEditScale(CEdit* pCtrl) 
{
	CString strText;
	if (!pCtrl)
		return;
	if (!pCtrl->m_hWnd)
		return;
	pCtrl->GetWindowText(strText);
	int iVal = (int)atoi ((LPCTSTR)strText);
	if (iVal < MINF8_SCALE)
	{
		MessageBeep (-1);
		strText.Format (_T("%d"), MINF8_SCALE);
		pCtrl->SetWindowText (strText);
	}
	else
	if (iVal > MAXF8_SCALE)
	{
		MessageBeep (-1);
		strText.Format (_T("%d"), MAXF8_SCALE);
		pCtrl->SetWindowText (strText);
	}
}