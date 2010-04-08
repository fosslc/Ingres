/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ipmprdta.cpp, Implementation file.
**
**
**    Project  : Visual DBA
**    Author   : UK Sotheavut
**
**    Purpose  : The implementation of serialization of IPM Page (modeless dialog)
**
**  History after 04-May-1999:
**
** 02-Aug-1999 (schph01)
**    bug #98166
**    Change the seralize number for macro
**    IMPLEMENT_SERIAL in CaReplicationStaticDataPageIntegrity class.
**    Add new variable member (m_DbList1,m_DbList2) in
**    CaReplicationStaticDataPageIntegrity::Serialize.
** 25-Jan-2000 (uk$so01)
**    SIR #100118: Add a description of the session.
** 06-Dec-2000 (schph01)
**    SIR 102822 change the IMPLEMENT_SERIAL number for SUMMARY: LogInfo
**    because the structure LPLOGINFOSUMMARY have change.
** 20-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "ipmprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_SERIAL (CuIpmPropertyData, CObject, 1)
CuIpmPropertyData::CuIpmPropertyData(){m_strClassName = _T("");}
CuIpmPropertyData::CuIpmPropertyData(LPCTSTR lpszClassName){m_strClassName = lpszClassName;}

void CuIpmPropertyData::NotifyLoad (CWnd* pDlg){}  
void CuIpmPropertyData::Serialize (CArchive& ar){}

//
// DETAIL: Location
// ****************
IMPLEMENT_SERIAL (CuIpmPropertyDataDetailLocation, CuIpmPropertyData, 1)
CuIpmPropertyDataDetailLocation::CuIpmPropertyDataDetailLocation()
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailLocation"))
{

}

void CuIpmPropertyDataDetailLocation::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)m_pPieInfo);
}

void CuIpmPropertyDataDetailLocation::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_pPieInfo;
	}
	else
	{
		ar >> m_pPieInfo;
	}
}


//
// PAGE: Location (Page 2, Page1 is call Detail)
// *********************************************
IMPLEMENT_SERIAL (CuIpmPropertyDataPageLocation, CuIpmPropertyData, 1)
CuIpmPropertyDataPageLocation::CuIpmPropertyDataPageLocation()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageLocation"))
{

}

void CuIpmPropertyDataPageLocation::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)m_pPieInfo);
}

void CuIpmPropertyDataPageLocation::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_pPieInfo; 
	}
	else
	{
		ar >> m_pPieInfo; 
	}
}


//
// PAGE: Location Usage (Page 3)
// *****************************
IMPLEMENT_SERIAL (CuIpmPropertyDataPageLocationUsage, CuIpmPropertyData, 1)
CuIpmPropertyDataPageLocationUsage::CuIpmPropertyDataPageLocationUsage()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageLocationUsage"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}


CuIpmPropertyDataPageLocationUsage::CuIpmPropertyDataPageLocationUsage(LPLOCATIONPARAMS pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageLocationUsage"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}
   

void CuIpmPropertyDataPageLocationUsage::NotifyLoad (CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}

void CuIpmPropertyDataPageLocationUsage::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar.Write (&m_dlgData, sizeof (m_dlgData)); 
	}
	else
	{
		ar.Read (&m_dlgData, sizeof (m_dlgData)); 
	}
}

//
// SUMMARY: Locations
// ******************
IMPLEMENT_SERIAL (CuIpmPropertyDataSummaryLocations, CuIpmPropertyData, 1)

CuIpmPropertyDataSummaryLocations::CuIpmPropertyDataSummaryLocations()
	:CuIpmPropertyData(_T("CuIpmPropertyDataSummaryLocations"))
{

}

void CuIpmPropertyDataSummaryLocations::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuIpmPropertyDataSummaryLocations::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_nBarWidth;
		ar << m_nUnit;
		ar << m_pDiskInfo; 
	}
	else
	{
		ar >> m_nBarWidth;
		ar >> m_nUnit;
		ar >> m_pDiskInfo; 
	}
}
