/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmicdta.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The implementation of serialization of IPM Page (modeless dialog)
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ipmicdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// DETAIL: ICE CONNECTED USER
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIceDetailConnectedUser, CuIpmPropertyData, 1)
CaIpmPropertyDataIceDetailConnectedUser::CaIpmPropertyDataIceDetailConnectedUser()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailConnectedUser"))
{
	memset (&m_dlgData, 0, sizeof(m_dlgData));
}


CaIpmPropertyDataIceDetailConnectedUser::CaIpmPropertyDataIceDetailConnectedUser (ICE_CONNECTED_USERDATAMIN* pData)
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailConnectedUser"))
{
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}


void CaIpmPropertyDataIceDetailConnectedUser::NotifyLoad (CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}


void CaIpmPropertyDataIceDetailConnectedUser::Serialize (CArchive& ar)
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
// DETAIL: ICE TRANSACTION
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIceDetailTransaction, CuIpmPropertyData, 1)
CaIpmPropertyDataIceDetailTransaction::CaIpmPropertyDataIceDetailTransaction()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailTransaction"))
{
	memset (&m_dlgData, 0, sizeof(m_dlgData));
}


CaIpmPropertyDataIceDetailTransaction::CaIpmPropertyDataIceDetailTransaction (ICE_TRANSACTIONDATAMIN* pData)
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailTransaction"))
{
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}


void CaIpmPropertyDataIceDetailTransaction::NotifyLoad (CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}


void CaIpmPropertyDataIceDetailTransaction::Serialize (CArchive& ar)
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
// DETAIL: ICE CURSOR
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIceDetailCursor, CuIpmPropertyData, 1)
CaIpmPropertyDataIceDetailCursor::CaIpmPropertyDataIceDetailCursor()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailCursor"))
{
	memset (&m_dlgData, 0, sizeof(m_dlgData));
}


CaIpmPropertyDataIceDetailCursor::CaIpmPropertyDataIceDetailCursor (ICE_CURSORDATAMIN* pData)
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailCursor"))
{
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}


void CaIpmPropertyDataIceDetailCursor::NotifyLoad (CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}


void CaIpmPropertyDataIceDetailCursor::Serialize (CArchive& ar)
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
// DETAIL: ICE CACHE PAGE
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIceDetailCachePage, CuIpmPropertyData, 1)
CaIpmPropertyDataIceDetailCachePage::CaIpmPropertyDataIceDetailCachePage()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailCachePage"))
{
	memset (&m_dlgData, 0, sizeof(m_dlgData));
}


CaIpmPropertyDataIceDetailCachePage::CaIpmPropertyDataIceDetailCachePage (ICE_CACHEINFODATAMIN* pData)
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailCachePage"))
{
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}


void CaIpmPropertyDataIceDetailCachePage::NotifyLoad (CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}


void CaIpmPropertyDataIceDetailCachePage::Serialize (CArchive& ar)
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
// DETAIL: ICE ACTIVE USER
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIceDetailActiveUser, CuIpmPropertyData, 1)
CaIpmPropertyDataIceDetailActiveUser::CaIpmPropertyDataIceDetailActiveUser()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailActiveUser"))
{
	memset (&m_dlgData, 0, sizeof(m_dlgData));
}


CaIpmPropertyDataIceDetailActiveUser::CaIpmPropertyDataIceDetailActiveUser (ICE_ACTIVE_USERDATAMIN* pData)
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailActiveUser"))
{
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}


void CaIpmPropertyDataIceDetailActiveUser::NotifyLoad (CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}


void CaIpmPropertyDataIceDetailActiveUser::Serialize (CArchive& ar)
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
// DETAIL: ICE HTTP SERVER CONNECTION
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIceDetailHttpServerConnection, CuIpmPropertyData, 1)
CaIpmPropertyDataIceDetailHttpServerConnection::CaIpmPropertyDataIceDetailHttpServerConnection()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailHttpServerConnection"))
{
	memset (&m_dlgData, 0, sizeof(m_dlgData));
}

CaIpmPropertyDataIceDetailHttpServerConnection::CaIpmPropertyDataIceDetailHttpServerConnection (SESSIONDATAMIN* pData)
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailHttpServerConnection"))
{
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}

void CaIpmPropertyDataIceDetailHttpServerConnection::NotifyLoad (CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}

void CaIpmPropertyDataIceDetailHttpServerConnection::Serialize (CArchive& ar)
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
// DETAIL: ICE DATABASE CONNECTION
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIceDetailDatabaseConnection, CuIpmPropertyData, 1)
CaIpmPropertyDataIceDetailDatabaseConnection::CaIpmPropertyDataIceDetailDatabaseConnection()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailDatabaseConnection"))
{
	memset (&m_dlgData, 0, sizeof(m_dlgData));
}


CaIpmPropertyDataIceDetailDatabaseConnection::CaIpmPropertyDataIceDetailDatabaseConnection (ICE_DB_CONNECTIONDATAMIN* pData)
	:CuIpmPropertyData(_T("CaIpmPropertyDataIceDetailDatabaseConnection"))
{
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}


void CaIpmPropertyDataIceDetailDatabaseConnection::NotifyLoad (CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}


void CaIpmPropertyDataIceDetailDatabaseConnection::Serialize (CArchive& ar)
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
// PAGE: ICE CONNECTED USER
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIcePageConnectedUser, CuIpmPropertyData, 1)
CaIpmPropertyDataIcePageConnectedUser::CaIpmPropertyDataIcePageConnectedUser()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIcePageConnectedUser"))
{
}

CaIpmPropertyDataIcePageConnectedUser::~CaIpmPropertyDataIcePageConnectedUser()
{
	while (!m_listTuple.IsEmpty())
	{
		ICE_CONNECTED_USERDATAMIN* pData = (ICE_CONNECTED_USERDATAMIN*)m_listTuple.RemoveHead();
		if (pData)
			delete pData;
	}
}

void CaIpmPropertyDataIcePageConnectedUser::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CaIpmPropertyDataIcePageConnectedUser::Serialize (CArchive& ar)
{
	ICE_CONNECTED_USERDATAMIN empyData;
	memset (&empyData, 0, sizeof(empyData));
	if (ar.IsStoring())
	{
		int nCount = m_listTuple.GetCount();
		ar << nCount;
		POSITION pos = m_listTuple.GetHeadPosition();
		while (pos != NULL)
		{
			ICE_CONNECTED_USERDATAMIN* pData = (ICE_CONNECTED_USERDATAMIN*)m_listTuple.GetNext (pos);
			//
			// In case of error:
			if (!pData)
				ar.Write (&empyData, sizeof (empyData));
			else
				ar.Write (pData, sizeof (empyData));
		}
	}
	else
	{
		int i, nCount = 0;
		ICE_CONNECTED_USERDATAMIN* pData = NULL;
		ar >> nCount;
		for (i=0; i<nCount; i++)
		{
			pData = new ICE_CONNECTED_USERDATAMIN;
			ar.Read (pData, sizeof (empyData));
			m_listTuple.AddTail (pData);
		}
	}
}



//
// PAGE: ICE TRANSACTION
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIcePageTransaction, CuIpmPropertyData, 1)
CaIpmPropertyDataIcePageTransaction::CaIpmPropertyDataIcePageTransaction()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIcePageTransaction"))
{
}

CaIpmPropertyDataIcePageTransaction::~CaIpmPropertyDataIcePageTransaction()
{
	while (!m_listTuple.IsEmpty())
	{
		ICE_TRANSACTIONDATAMIN* pData = (ICE_TRANSACTIONDATAMIN*)m_listTuple.RemoveHead();
		if (pData)
			delete pData;
	}
}

void CaIpmPropertyDataIcePageTransaction::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CaIpmPropertyDataIcePageTransaction::Serialize (CArchive& ar)
{
	ICE_TRANSACTIONDATAMIN empyData;
	memset (&empyData, 0, sizeof(empyData));
	if (ar.IsStoring())
	{
		int nCount = m_listTuple.GetCount();
		ar << nCount;
		POSITION pos = m_listTuple.GetHeadPosition();
		while (pos != NULL)
		{
			ICE_TRANSACTIONDATAMIN* pData = (ICE_TRANSACTIONDATAMIN*)m_listTuple.GetNext (pos);
			//
			// In case of error:
			if (!pData)
				ar.Write (&empyData, sizeof (empyData));
			else
				ar.Write (pData, sizeof (empyData));
		}
	}
	else
	{
		int i, nCount = 0;
		ICE_TRANSACTIONDATAMIN* pData = NULL;
		ar >> nCount;
		for (i=0; i<nCount; i++)
		{
			pData = new ICE_TRANSACTIONDATAMIN;
			ar.Read (pData, sizeof (empyData));
			m_listTuple.AddTail (pData);
		}
	}
}


//
// PAGE: ICE CURSOR
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIcePageCursor, CuIpmPropertyData, 1)
CaIpmPropertyDataIcePageCursor::CaIpmPropertyDataIcePageCursor()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIcePageCursor"))
{
}

CaIpmPropertyDataIcePageCursor::~CaIpmPropertyDataIcePageCursor()
{
	while (!m_listTuple.IsEmpty())
	{
		ICE_CURSORDATAMIN* pData = (ICE_CURSORDATAMIN*)m_listTuple.RemoveHead();
		if (pData)
			delete pData;
	}
}

void CaIpmPropertyDataIcePageCursor::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CaIpmPropertyDataIcePageCursor::Serialize (CArchive& ar)
{
	ICE_CURSORDATAMIN empyData;
	memset (&empyData, 0, sizeof(empyData));
	if (ar.IsStoring())
	{
		int nCount = m_listTuple.GetCount();
		ar << nCount;
		POSITION pos = m_listTuple.GetHeadPosition();
		while (pos != NULL)
		{
			ICE_CURSORDATAMIN* pData = (ICE_CURSORDATAMIN*)m_listTuple.GetNext (pos);
			//
			// In case of error:
			if (!pData)
				ar.Write (&empyData, sizeof (empyData));
			else
				ar.Write (pData, sizeof (empyData));
		}
	}
	else
	{
		int i, nCount = 0;
		ICE_CURSORDATAMIN* pData = NULL;
		ar >> nCount;
		for (i=0; i<nCount; i++)
		{
			pData = new ICE_CURSORDATAMIN;
			ar.Read (pData, sizeof (empyData));
			m_listTuple.AddTail (pData);
		}
	}
}



//
// PAGE: ICE CACHE PAGE
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIcePageCachePage, CuIpmPropertyData, 1)
CaIpmPropertyDataIcePageCachePage::CaIpmPropertyDataIcePageCachePage()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIcePageCachePage"))
{
}

CaIpmPropertyDataIcePageCachePage::~CaIpmPropertyDataIcePageCachePage()
{
	while (!m_listTuple.IsEmpty())
	{
		ICE_CACHEINFODATAMIN* pData = (ICE_CACHEINFODATAMIN*)m_listTuple.RemoveHead();
		if (pData)
			delete pData;
	}
}

void CaIpmPropertyDataIcePageCachePage::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CaIpmPropertyDataIcePageCachePage::Serialize (CArchive& ar)
{
	ICE_CACHEINFODATAMIN empyData;
	memset (&empyData, 0, sizeof(empyData));
	if (ar.IsStoring())
	{
		int nCount = m_listTuple.GetCount();
		ar << nCount;
		POSITION pos = m_listTuple.GetHeadPosition();
		while (pos != NULL)
		{
			ICE_CACHEINFODATAMIN* pData = (ICE_CACHEINFODATAMIN*)m_listTuple.GetNext (pos);
			//
			// In case of error:
			if (!pData)
				ar.Write (&empyData, sizeof (empyData));
			else
				ar.Write (pData, sizeof (empyData));
		}
	}
	else
	{
		int i, nCount = 0;
		ICE_CACHEINFODATAMIN* pData = NULL;
		ar >> nCount;
		for (i=0; i<nCount; i++)
		{
			pData = new ICE_CACHEINFODATAMIN;
			ar.Read (pData, sizeof (empyData));
			m_listTuple.AddTail (pData);
		}
	}
}



//
// PAGE: ICE ACTIVE USER
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIcePageActiveUser, CuIpmPropertyData, 1)
CaIpmPropertyDataIcePageActiveUser::CaIpmPropertyDataIcePageActiveUser()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIcePageActiveUser"))
{
}

CaIpmPropertyDataIcePageActiveUser::~CaIpmPropertyDataIcePageActiveUser()
{
	while (!m_listTuple.IsEmpty())
	{
		ICE_ACTIVE_USERDATAMIN* pData = (ICE_ACTIVE_USERDATAMIN*)m_listTuple.RemoveHead();
		if (pData)
			delete pData;
	}
}

void CaIpmPropertyDataIcePageActiveUser::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CaIpmPropertyDataIcePageActiveUser::Serialize (CArchive& ar)
{
	ICE_ACTIVE_USERDATAMIN empyData;
	memset (&empyData, 0, sizeof(empyData));
	if (ar.IsStoring())
	{
		int nCount = m_listTuple.GetCount();
		ar << nCount;
		POSITION pos = m_listTuple.GetHeadPosition();
		while (pos != NULL)
		{
			ICE_ACTIVE_USERDATAMIN* pData = (ICE_ACTIVE_USERDATAMIN*)m_listTuple.GetNext (pos);
			//
			// In case of error:
			if (!pData)
				ar.Write (&empyData, sizeof (empyData));
			else
				ar.Write (pData, sizeof (empyData));
		}
	}
	else
	{
		int i, nCount = 0;
		ICE_ACTIVE_USERDATAMIN* pData = NULL;
		ar >> nCount;
		for (i=0; i<nCount; i++)
		{
			pData = new ICE_ACTIVE_USERDATAMIN;
			ar.Read (pData, sizeof (empyData));
			m_listTuple.AddTail (pData);
		}
	}
}


//
// PAGE: ICE HTTP SERVER CONNECTION
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIcePageHttpServerConnection, CuIpmPropertyData, 1)
CaIpmPropertyDataIcePageHttpServerConnection::CaIpmPropertyDataIcePageHttpServerConnection()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIcePageHttpServerConnection"))
{
}

CaIpmPropertyDataIcePageHttpServerConnection::~CaIpmPropertyDataIcePageHttpServerConnection()
{
	while (!m_listTuple.IsEmpty())
	{
		SESSIONDATAMIN* pData = (SESSIONDATAMIN*)m_listTuple.RemoveHead();
		if (pData)
			delete pData;
	}
}

void CaIpmPropertyDataIcePageHttpServerConnection::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CaIpmPropertyDataIcePageHttpServerConnection::Serialize (CArchive& ar)
{
	SESSIONDATAMIN empyData;
	memset (&empyData, 0, sizeof(empyData));
	if (ar.IsStoring())
	{
		int nCount = m_listTuple.GetCount();
		ar << nCount;
		POSITION pos = m_listTuple.GetHeadPosition();
		while (pos != NULL)
		{
			SESSIONDATAMIN* pData = (SESSIONDATAMIN*)m_listTuple.GetNext (pos);
			//
			// In case of error:
			if (!pData)
				ar.Write (&empyData, sizeof (empyData));
			else
				ar.Write (pData, sizeof (empyData));
		}
	}
	else
	{
		int i, nCount = 0;
		SESSIONDATAMIN* pData = NULL;
		ar >> nCount;
		for (i=0; i<nCount; i++)
		{
			pData = new SESSIONDATAMIN;
			ar.Read (pData, sizeof (empyData));
			m_listTuple.AddTail (pData);
		}
	}
}



//
// PAGE: ICE DATABASE CONNECTION
// ***************************************************************
IMPLEMENT_SERIAL (CaIpmPropertyDataIcePageDatabaseConnection, CuIpmPropertyData, 1)
CaIpmPropertyDataIcePageDatabaseConnection::CaIpmPropertyDataIcePageDatabaseConnection()
	:CuIpmPropertyData(_T("CaIpmPropertyDataIcePageDatabaseConnection"))
{
}

CaIpmPropertyDataIcePageDatabaseConnection::~CaIpmPropertyDataIcePageDatabaseConnection()
{
	while (!m_listTuple.IsEmpty())
	{
		ICE_DB_CONNECTIONDATAMIN* pData = (ICE_DB_CONNECTIONDATAMIN*)m_listTuple.RemoveHead();
		if (pData)
			delete pData;
	}
}

void CaIpmPropertyDataIcePageDatabaseConnection::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CaIpmPropertyDataIcePageDatabaseConnection::Serialize (CArchive& ar)
{
	ICE_DB_CONNECTIONDATAMIN empyData;
	memset (&empyData, 0, sizeof(empyData));
	if (ar.IsStoring())
	{
		int nCount = m_listTuple.GetCount();
		ar << nCount;
		POSITION pos = m_listTuple.GetHeadPosition();
		while (pos != NULL)
		{
			ICE_DB_CONNECTIONDATAMIN* pData = (ICE_DB_CONNECTIONDATAMIN*)m_listTuple.GetNext (pos);
			//
			// In case of error:
			if (!pData)
				ar.Write (&empyData, sizeof (empyData));
			else
				ar.Write (pData, sizeof (empyData));
		}
	}
	else
	{
		int i, nCount = 0;
		ICE_DB_CONNECTIONDATAMIN* pData = NULL;
		ar >> nCount;
		for (i=0; i<nCount; i++)
		{
			pData = new ICE_DB_CONNECTIONDATAMIN;
			ar.Read (pData, sizeof (empyData));
			m_listTuple.AddTail (pData);
		}
	}
}

