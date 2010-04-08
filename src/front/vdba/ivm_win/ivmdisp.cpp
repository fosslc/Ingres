/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : ivmdisp.h, Implementation file
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : CaPageInformation, maintains the information for displaying
**               the property of Ingres Visual Manager
**               It contain the number of pages, the Dialog ID, the Label for the
**               TabCtrl, .., for each item data of IVM Item
**               CaIvmProperty is maintained for keeping track of the object being
**               displayed in the TabCtrl, its current page,
**
** History:
**
** 15-Dec-1998 (uk$so01)
**    created
** 06-Jun-2000 (uk$so01)
**    bug 99242 Handle DBCS
**
**/

#include "stdafx.h"
#include "ivmdisp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// CaPageInformation Implementation
// ********************************
CaPageInformation::CaPageInformation()
{
	m_strTitle     = _T("");
	m_strClassName = _T(""); 
	m_nNumber      = 0;  
	m_pTabID       = NULL;
	m_pDlgID       = NULL;
	m_pIvmItem     = NULL;
	m_nDefaultPage = 0;
}

CaPageInformation::CaPageInformation(CString strClass, int nSize, UINT* tabArray, UINT* dlgArray, UINT nIDTitle)
{
	m_strTitle     = _T("");
	m_strClassName = strClass; 
	m_nNumber      = nSize;  
	m_pTabID       = NULL;
	m_pDlgID       = NULL;  
	m_pIvmItem     = NULL;
	m_nIDTitle     = nIDTitle;
	if (m_nNumber > 0)
	{
		m_pTabID = new UINT [m_nNumber];
		m_pDlgID = new UINT [m_nNumber];
		for (int i=0; i<m_nNumber; i++)
		{
			m_pTabID [i] = tabArray [i];
			m_pDlgID [i] = dlgArray [i];
		}
	}
	m_nDefaultPage = 0;
}


CaPageInformation::CaPageInformation(CString strClass)
{
	m_strTitle     = _T("");
	m_strClassName = strClass; 
	m_nNumber      = 0;  
	m_pTabID       = NULL;
	m_pDlgID       = NULL;
	m_nIDTitle     = 0;
}

CaPageInformation::~CaPageInformation()
{
	if (m_pTabID)
		delete[] m_pTabID;
	if (m_pDlgID)
		delete[] m_pDlgID;
}



CString CaPageInformation::GetClassName()   {return m_strClassName;}
int     CaPageInformation::GetNumberOfPage(){return m_nNumber;}
UINT*   CaPageInformation::GetTabID ()      {return m_pTabID;}
UINT*   CaPageInformation::GetDlgID ()      {return m_pDlgID;}
UINT    CaPageInformation::GetTabID (int nIndex)
{
	ASSERT(nIndex>=0 && nIndex <m_nNumber); 
	return m_pTabID[nIndex];
}

UINT CaPageInformation::GetDlgID (int nIndex)
{
	ASSERT(nIndex>=0 && nIndex <m_nNumber); 
	return m_pDlgID[nIndex];
}

void CaPageInformation::GetTitle (CString& strTile)
{
	if (m_nIDTitle == 0)
		strTile = _T("");
	else
		strTile = m_strTitle;
}



//
// CaIvmProperty Implementation
// ****************************
CaIvmProperty::CaIvmProperty()
{
	m_nCurrentSelected = -1;
	m_pPageInfo = NULL;
}

CaIvmProperty::CaIvmProperty(int nCurSel, CaPageInformation*  pPageInfo)
{
	m_nCurrentSelected = nCurSel;
	m_pPageInfo = pPageInfo;
}

CaIvmProperty::~CaIvmProperty()
{

}

void CaIvmProperty::SetCurSel  (int nSel) {m_nCurrentSelected = nSel;}
void CaIvmProperty::SetPageInfo(CaPageInformation*  pPageInfo){m_pPageInfo = pPageInfo;}
int  CaIvmProperty::GetCurSel (){return m_nCurrentSelected;}
CaPageInformation* CaIvmProperty::GetPageInfo(){return m_pPageInfo;}

