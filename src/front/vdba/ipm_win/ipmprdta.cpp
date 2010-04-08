/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmprdta.cpp, Implementation file.
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The implementation of serialization of IPM Page (modeless dialog) 
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
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
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ipmprdta.h"
#include "statfrm.h"

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
// DETAIL: Active User
// *******************
IMPLEMENT_SERIAL (CuIpmPropertyDataDetailActiveUser, CuIpmPropertyData, 1)
CuIpmPropertyDataDetailActiveUser::CuIpmPropertyDataDetailActiveUser()
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailActiveUser"))
{

}

void CuIpmPropertyDataDetailActiveUser::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, 0, 0);
}

void CuIpmPropertyDataDetailActiveUser::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{


	}
	else
	{

	}
}


//
// DETAIL: Database
// ****************
IMPLEMENT_SERIAL (CuIpmPropertyDataDetailDatabase, CuIpmPropertyData, 1)
CuIpmPropertyDataDetailDatabase::CuIpmPropertyDataDetailDatabase()
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailDatabase"))
{

}


void CuIpmPropertyDataDetailDatabase::NotifyLoad (CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, 0, 0);
}

void CuIpmPropertyDataDetailDatabase::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{


	}
	else
	{

	}
}


//
// DETAIL: Lock
// ************
IMPLEMENT_SERIAL (CuIpmPropertyDataDetailLock, CuIpmPropertyData, 1)
CuIpmPropertyDataDetailLock::CuIpmPropertyDataDetailLock()
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailLock"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}

CuIpmPropertyDataDetailLock::CuIpmPropertyDataDetailLock (LPLOCKDATAMAX pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailLock"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}

void CuIpmPropertyDataDetailLock::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}

void CuIpmPropertyDataDetailLock::Serialize (CArchive& ar)
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
// DETAIL: Lock List
// *****************
IMPLEMENT_SERIAL (CuIpmPropertyDataDetailLockList, CuIpmPropertyData, 1)
CuIpmPropertyDataDetailLockList::CuIpmPropertyDataDetailLockList()
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailLockList"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}

CuIpmPropertyDataDetailLockList::CuIpmPropertyDataDetailLockList (LPLOCKLISTDATAMAX pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailLockList"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}

void CuIpmPropertyDataDetailLockList::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}

void CuIpmPropertyDataDetailLockList::Serialize (CArchive& ar)
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
// DETAIL: Process
// ***************
IMPLEMENT_SERIAL (CuIpmPropertyDataDetailProcess, CuIpmPropertyData, 1)
CuIpmPropertyDataDetailProcess::CuIpmPropertyDataDetailProcess()
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailProcess"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}

CuIpmPropertyDataDetailProcess::CuIpmPropertyDataDetailProcess (LPLOGPROCESSDATAMAX pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailProcess"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}

void CuIpmPropertyDataDetailProcess::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}

void CuIpmPropertyDataDetailProcess::Serialize (CArchive& ar)
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
// DETAIL: Resource
// ****************
IMPLEMENT_SERIAL (CuIpmPropertyDataDetailResource, CuIpmPropertyData, 1)
CuIpmPropertyDataDetailResource::CuIpmPropertyDataDetailResource()
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailResource"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}

CuIpmPropertyDataDetailResource::CuIpmPropertyDataDetailResource (LPRESOURCEDATAMAX pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailResource"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}

void CuIpmPropertyDataDetailResource::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}

void CuIpmPropertyDataDetailResource::Serialize (CArchive& ar)
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
// DETAIL: CaIpmPropertyNetTraficData
// **********************************
IMPLEMENT_SERIAL (CaIpmPropertyNetTraficData, CObject, 1)
CaIpmPropertyNetTraficData::CaIpmPropertyNetTraficData()
{
	m_bStartup = TRUE;
	m_nCounter = 0; 
	m_strTimeStamp = _T("");

	m_strTotalPacketIn = _T("");
	m_strTotalDataIn = _T("");
	m_strTotalPacketOut = _T("");
	m_strTotalDataOut = _T("");
	m_strCurrentPacketIn = _T("");
	m_strCurrentDataIn = _T("");
	m_strCurrentPacketOut = _T("");
	m_strCurrentDataOut = _T("");
	m_strAveragePacketIn = _T("");
	m_strAverageDataIn = _T("");
	m_strAveragePacketOut = _T("");
	m_strAverageDataOut = _T("");
	m_strPeekPacketIn = _T("");
	m_strPeekDataIn = _T("");
	m_strPeekPacketOut = _T("");
	m_strPeekDataOut = _T("");
	m_strDate1PacketIn = _T("");
	m_strDate2PacketIn = _T("");
	m_strDate1DataIn = _T("");
	m_strDate2DataIn = _T("");
	m_strDate1PacketOut = _T("");
	m_strDate2PacketOut = _T("");
	m_strDate1DataOut = _T("");
	m_strDate2DataOut = _T("");
}

void CaIpmPropertyNetTraficData::Serialize (CArchive& ar)
{
	m_TraficInfo.Serialize(ar);
	if (ar.IsStoring())
	{
		ar << m_bStartup;
		ar << m_nCounter; 
		ar << m_strTimeStamp;

		ar << m_strTotalPacketIn;
		ar << m_strTotalDataIn;
		ar << m_strTotalPacketOut;
		ar << m_strTotalDataOut;
		ar << m_strCurrentPacketIn;
		ar << m_strCurrentDataIn;
		ar << m_strCurrentPacketOut;
		ar << m_strCurrentDataOut;
		ar << m_strAveragePacketIn;
		ar << m_strAverageDataIn;
		ar << m_strAveragePacketOut;
		ar << m_strAverageDataOut;
		ar << m_strPeekPacketIn;
		ar << m_strPeekDataIn;
		ar << m_strPeekPacketOut;
		ar << m_strPeekDataOut;
		ar << m_strDate1PacketIn;
		ar << m_strDate2PacketIn;
		ar << m_strDate1DataIn;
		ar << m_strDate2DataIn;
		ar << m_strDate1PacketOut;
		ar << m_strDate2PacketOut;
		ar << m_strDate1DataOut;
		ar << m_strDate2DataOut;
	}
	else
	{
		ar >> m_bStartup;
		ar >> m_nCounter; 
		ar >> m_strTimeStamp;

		ar >> m_strTotalPacketIn;
		ar >> m_strTotalDataIn;
		ar >> m_strTotalPacketOut;
		ar >> m_strTotalDataOut;
		ar >> m_strCurrentPacketIn;
		ar >> m_strCurrentDataIn;
		ar >> m_strCurrentPacketOut;
		ar >> m_strCurrentDataOut;
		ar >> m_strAveragePacketIn;
		ar >> m_strAverageDataIn;
		ar >> m_strAveragePacketOut;
		ar >> m_strAverageDataOut;
		ar >> m_strPeekPacketIn;
		ar >> m_strPeekDataIn;
		ar >> m_strPeekPacketOut;
		ar >> m_strPeekDataOut;
		ar >> m_strDate1PacketIn;
		ar >> m_strDate2PacketIn;
		ar >> m_strDate1DataIn;
		ar >> m_strDate2DataIn;
		ar >> m_strDate1PacketOut;
		ar >> m_strDate2PacketOut;
		ar >> m_strDate1DataOut;
		ar >> m_strDate2DataOut;
	}
}

//
// DETAIL: Server
// ****************
IMPLEMENT_SERIAL (CuIpmPropertyDataDetailServer, CuIpmPropertyData, 1)
CuIpmPropertyDataDetailServer::CuIpmPropertyDataDetailServer()
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailServer"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}

CuIpmPropertyDataDetailServer::CuIpmPropertyDataDetailServer (LPSERVERDATAMAX pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailServer"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}

void CuIpmPropertyDataDetailServer::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuIpmPropertyDataDetailServer::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar.Write (&m_dlgData, sizeof (m_dlgData)); 
	}
	else
	{
		ar.Read (&m_dlgData, sizeof (m_dlgData));
	}
	m_netTraficData.Serialize(ar);
}


//
// DETAIL: Session
// ***************
IMPLEMENT_SERIAL (CuIpmPropertyDataDetailSession, CuIpmPropertyData, 2)
CuIpmPropertyDataDetailSession::CuIpmPropertyDataDetailSession()
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailSession"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}

CuIpmPropertyDataDetailSession::CuIpmPropertyDataDetailSession (LPSESSIONDATAMAX pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailSession"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}

void CuIpmPropertyDataDetailSession::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}

void CuIpmPropertyDataDetailSession::Serialize (CArchive& ar)
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
// DETAIL: Transaction
// *******************
IMPLEMENT_SERIAL (CuIpmPropertyDataDetailTransaction, CuIpmPropertyData, 1)
CuIpmPropertyDataDetailTransaction::CuIpmPropertyDataDetailTransaction()
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailTransaction"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}

CuIpmPropertyDataDetailTransaction::CuIpmPropertyDataDetailTransaction (LPLOGTRANSACTDATAMAX pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataDetailTransaction"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}

void CuIpmPropertyDataDetailTransaction::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}

void CuIpmPropertyDataDetailTransaction::Serialize (CArchive& ar)
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
// PAGE: Active Databases
//		 tuple = (DB, Status, TXCnt, Begin, End, Read, Write)
//		 It must containt the list of tuples.
// *********************************************************
IMPLEMENT_SERIAL (CuDataTupleActiveDatabase, CObject, 1)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageActiveDatabases, CuIpmPropertyData, 1)

CuDataTupleActiveDatabase::CuDataTupleActiveDatabase()
{
	m_strDatabase= _T("");
	m_strStatus = _T("");
	m_strTXCnt= _T("");
	m_strBegin= _T("");
	m_strEnd= _T("");
	m_strRead= _T("");
	m_strWrite= _T("");
}

CuDataTupleActiveDatabase::CuDataTupleActiveDatabase(
	LPCTSTR lpszDatabase,
	LPCTSTR lpszStatus, 
	LPCTSTR lpszTXCnt, 
	LPCTSTR lpszBegin,
	LPCTSTR lpszEnd,
	LPCTSTR lpszRead,
	LPCTSTR lpszWrite)
{
	m_strDatabase= lpszDatabase;
	m_strStatus= lpszStatus;
	m_strTXCnt= lpszTXCnt;
	m_strBegin= lpszBegin;
	m_strEnd= lpszEnd;
	m_strRead= lpszRead;
	m_strWrite= lpszWrite; 
}

void CuDataTupleActiveDatabase::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strDatabase;
		ar << m_strStatus;
		ar << m_strTXCnt;
		ar << m_strBegin;
		ar << m_strEnd; 
		ar << m_strRead;
		ar << m_strWrite;
	}
	else
	{
		ar >> m_strDatabase;
		ar >> m_strStatus;
		ar >> m_strTXCnt;
		ar >> m_strBegin;
		ar >> m_strEnd; 
		ar >> m_strRead;
		ar >> m_strWrite;
	}
}

CuIpmPropertyDataPageActiveDatabases::CuIpmPropertyDataPageActiveDatabases()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageActiveDatabases"))
{

}

CuIpmPropertyDataPageActiveDatabases::~CuIpmPropertyDataPageActiveDatabases()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageActiveDatabases::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageActiveDatabases::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}

//
// PAGE: Client
// ************
IMPLEMENT_SERIAL (CuIpmPropertyDataPageClient, CuIpmPropertyData, 1)
CuIpmPropertyDataPageClient::CuIpmPropertyDataPageClient()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageClient"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}

CuIpmPropertyDataPageClient::CuIpmPropertyDataPageClient (LPSESSIONDATAMAX pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageClient"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}

void CuIpmPropertyDataPageClient::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}

void CuIpmPropertyDataPageClient::Serialize (CArchive& ar)
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
// PAGE: Databases
// ***************
IMPLEMENT_SERIAL (CuIpmPropertyDataPageDatabases, CuIpmPropertyData, 1)
CuIpmPropertyDataPageDatabases::CuIpmPropertyDataPageDatabases()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageDatabases"))
{

}

void CuIpmPropertyDataPageDatabases::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)0, (LPARAM)0);
}

void CuIpmPropertyDataPageDatabases::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		 
	}
	else
	{
		
	}
}


//
// PAGE: Headers
// *************
IMPLEMENT_SERIAL (CuIpmPropertyDataPageHeaders, CuIpmPropertyData, 1)

CuIpmPropertyDataPageHeaders::CuIpmPropertyDataPageHeaders()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageHeaders"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}

CuIpmPropertyDataPageHeaders::CuIpmPropertyDataPageHeaders (LPLOGHEADERDATAMIN pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageHeaders"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}


void CuIpmPropertyDataPageHeaders::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}

void CuIpmPropertyDataPageHeaders::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar.Write (&m_dlgData, sizeof(m_dlgData)); 
	}
	else
	{
		ar.Read (&m_dlgData, sizeof(m_dlgData)); 
	}
}


// PAGE: Logfile
// *************
IMPLEMENT_SERIAL (CuIpmPropertyDataPageLogFile, CuIpmPropertyData, 1)
CuIpmPropertyDataPageLogFile::CuIpmPropertyDataPageLogFile()
	:CuIpmPropertyData()
{
	m_strClassName = _T("CuIpmPropertyDataPageLogFile");
	m_pPieInfo = NULL;
}

void CuIpmPropertyDataPageLogFile::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuIpmPropertyDataPageLogFile::Serialize (CArchive& ar)
{
	CuIpmPropertyData::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_strStatus; 
		ar << m_pPieInfo; 
	}
	else
	{
		ar >> m_strStatus; 
		ar >> m_pPieInfo; 
	}
}


//
// PAGE: Lock Lists
//		 tuple = (ID, Session, Count, Logical, MaxL, Status)
//		 It must containt the list of tuples.
// *********************************************************
IMPLEMENT_SERIAL (CuDataTupleLockList, CObject, 1)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageLockLists, CuIpmPropertyData, 1)

CuDataTupleLockList::CuDataTupleLockList()
{
	m_strID = _T("");
	m_strSession= _T("");
	m_strCount= _T("");
	m_strLogical= _T("");
	m_strMaxL= _T("");
	m_strStatus = _T("");
}

CuDataTupleLockList::CuDataTupleLockList(
	LPCTSTR lpszID,
	LPCTSTR lpszSession, 
	LPCTSTR lpszCount, 
	LPCTSTR lpszLogical,
	LPCTSTR lpszMaxL,
	LPCTSTR lpszStatus)
{
	m_strID = lpszID;
	m_strSession= lpszSession;
	m_strCount= lpszCount;
	m_strLogical= lpszLogical;
	m_strMaxL= lpszMaxL; 
	m_strStatus = lpszStatus; 
}
 
void CuDataTupleLockList::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strID; 
		ar << m_strSession;
		ar << m_strCount;  
		ar << m_strLogical;
		ar << m_strMaxL;   
		ar << m_strStatus;
	}
	else
	{
		ar >> m_strID; 
		ar >> m_strSession;
		ar >> m_strCount;  
		ar >> m_strLogical;
		ar >> m_strMaxL;   
		ar >> m_strStatus; 
	}
}


CuIpmPropertyDataPageLockLists::CuIpmPropertyDataPageLockLists()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageLockLists"))
{

}
	
CuIpmPropertyDataPageLockLists::~CuIpmPropertyDataPageLockLists()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageLockLists::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageLockLists::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}


//
// PAGE: Lock Resources
//		 tuple = (ID, GR, CV, LockType, DB, Table, Page)
//		 It must containt the list of tuples.
// ***************************************************************
IMPLEMENT_SERIAL (CuDataTupleResource, CObject, 1)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageLockResources, CuIpmPropertyData, 1)

CuDataTupleResource::CuDataTupleResource()
{
	m_strID = _T("");
	m_strGR = _T("");
	m_strCV = _T("");
	m_strLockType = _T("");
	m_strDB = _T("");  
	m_strTable = _T("");
	m_strPage = _T("");
}

CuDataTupleResource::CuDataTupleResource(
	LPCTSTR lpszID,
	LPCTSTR lpszGR, 
	LPCTSTR lpszCV, 
	LPCTSTR lpszLockType,
	LPCTSTR lpszDB,
	LPCTSTR lpszTable,
	LPCTSTR lpszPage)
{
	m_strID = lpszID;
	m_strGR = lpszGR;
	m_strCV = lpszCV;
	m_strLockType = lpszLockType;
	m_strDB = lpszDB;  
	m_strTable = lpszTable;
	m_strPage = lpszPage;
}

void CuDataTupleResource::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strID; 
		ar << m_strGR;
		ar << m_strCV;
		ar << m_strLockType;
		ar << m_strDB;
		ar << m_strTable;
		ar << m_strPage;
	}
	else
	{
		ar >> m_strID;
		ar >> m_strGR;
		ar >> m_strCV;
		ar >> m_strLockType;
		ar >> m_strDB;
		ar >> m_strTable;
		ar >> m_strPage;
	}
}

CuIpmPropertyDataPageLockResources::CuIpmPropertyDataPageLockResources()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageLockResources"))
{

}

CuIpmPropertyDataPageLockResources::~CuIpmPropertyDataPageLockResources()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageLockResources::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageLockResources::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}

//
// PAGE: Locks
//		 tuple = (ID, RQ, State, LockType, DB, Table, Page)
//		 It must containt the list of tuples.
// ***************************************************************
IMPLEMENT_SERIAL (CuDataTupleLock, CObject, 1)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageLocks, CuIpmPropertyData, 1)

CuDataTupleLock::CuDataTupleLock()
{
	m_strID = _T("");
	m_strRQ = _T("");
	m_strState = _T("");
	m_strLockType = _T("");
	m_strDB = _T("");  
	m_strTable = _T("");
	m_strPage = _T("");
}

CuDataTupleLock::CuDataTupleLock(
	LPCTSTR lpszID,
	LPCTSTR lpszRQ, 
	LPCTSTR lpszState, 
	LPCTSTR lpszLockType,
	LPCTSTR lpszDB,
	LPCTSTR lpszTable,
	LPCTSTR lpszPage)
{
	m_strID = lpszID;
	m_strRQ = lpszRQ;
	m_strState = lpszState;
	m_strLockType = lpszLockType;
	m_strDB = lpszDB;  
	m_strTable = lpszTable;
	m_strPage = lpszPage;
}

void CuDataTupleLock::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strID;
		ar << m_strRQ;
		ar << m_strState;
		ar << m_strLockType;
		ar << m_strDB;	
		ar << m_strTable;
		ar << m_strPage;
	}
	else
	{
		ar >> m_strID;
		ar >> m_strRQ;
		ar >> m_strState;
		ar >> m_strLockType;
		ar >> m_strDB;
		ar >> m_strTable;
		ar >> m_strPage;
	}
}

CuIpmPropertyDataPageLocks::CuIpmPropertyDataPageLocks()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageLocks"))
{

}

CuIpmPropertyDataPageLocks::~CuIpmPropertyDataPageLocks()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageLocks::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageLocks::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}


//
// PAGE: Lock Tables
//		 tuple = (ID, GR, CV, Table)
//		 It must containt the list of tuples.
// ***************************************************************
IMPLEMENT_SERIAL (CuDataTupleLockTable, CObject, 1)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageLockTables, CuIpmPropertyData, 1)


CuDataTupleLockTable::CuDataTupleLockTable()
{
	m_strID = _T("");
	m_strGR = _T("");
	m_strCV = _T("");
	m_strTable = _T("");
}

CuDataTupleLockTable::CuDataTupleLockTable(
	LPCTSTR lpszID,
	LPCTSTR lpszGR, 
	LPCTSTR lpszCV, 
	LPCTSTR lpszTable)
{
	m_strID = lpszID;
	m_strGR = lpszGR;
	m_strCV = lpszCV;
	m_strTable = lpszTable;
}

void CuDataTupleLockTable::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strID;
		ar << m_strGR;
		ar << m_strCV;
		ar << m_strTable;
	}
	else
	{
		ar >> m_strID;
		ar >> m_strGR;
		ar >> m_strCV;
		ar >> m_strTable;
	}
}

CuIpmPropertyDataPageLockTables::CuIpmPropertyDataPageLockTables()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageLockTables"))
{

}

CuIpmPropertyDataPageLockTables::~CuIpmPropertyDataPageLockTables()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageLockTables::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageLockTables::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}



//
// PAGE: Processes
// tuple = (ID, PID, Name, TTY, Database, State, Facil, Query)
// It must containt the list of tuples.
// *****************************************************************
IMPLEMENT_SERIAL (CuDataTupleProcess, CObject, 1)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageProcesses, CuIpmPropertyData, 1)

CuDataTupleProcess::CuDataTupleProcess()
{
	m_strID = _T("");
	m_strPID = _T("");
	m_strType = _T("");
	m_strOpenDB = _T("");
	m_strWrite = _T("");
	m_strForce = _T("");
	m_strWait = _T("");
	m_strBegin = _T("");
	m_strEnd = _T("");
}

CuDataTupleProcess::CuDataTupleProcess(
	LPCTSTR lpszID,
	LPCTSTR lpszPID, 
	LPCTSTR lpszType, 
	LPCTSTR lpszOpenDB,
	LPCTSTR lpszWrite,
	LPCTSTR lpszForce,
	LPCTSTR lpszWait,
	LPCTSTR lpszBegin,
	LPCTSTR lpszEnd)
{
	m_strID = lpszID;
	m_strPID = lpszPID;
	m_strType = lpszType;
	m_strOpenDB = lpszOpenDB;
	m_strWrite = lpszWrite;
	m_strForce = lpszForce;
	m_strWait = lpszWait;
	m_strBegin = lpszBegin;
	m_strEnd = lpszEnd;
}

void CuDataTupleProcess::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strID;
		ar << m_strPID;
		ar << m_strType ;
		ar << m_strOpenDB ;
		ar << m_strWrite  ;
		ar << m_strForce  ;
		ar << m_strWait   ;
		ar << m_strBegin  ;
		ar << m_strEnd ;
	}
	else
	{
		ar >> m_strID ;
		ar >> m_strPID ;
		ar >> m_strType ;
		ar >> m_strOpenDB ;
		ar >> m_strWrite  ;
		ar >> m_strForce  ;
		ar >> m_strWait   ;
		ar >> m_strBegin  ;
		ar >> m_strEnd;
	}
}

CuIpmPropertyDataPageProcesses::CuIpmPropertyDataPageProcesses()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageProcesses"))
{

}

CuIpmPropertyDataPageProcesses::~CuIpmPropertyDataPageProcesses()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageProcesses::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageProcesses::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}



//
// PAGE: Sessions
//       tuple = (ID, Name, TTY, Database, State, Facil, Query)
//       It must containt the list of tuples.
// *****************************************************************
IMPLEMENT_SERIAL (CuDataTupleSession, CObject, 1)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageSessions, CuIpmPropertyData, 1)

CuDataTupleSession::CuDataTupleSession()
{
	m_strID = _T("");
	m_strName = _T("");
	m_strTTY = _T("");
	m_strDatabase = _T("");
	m_strState = _T("");
	m_strFacil = _T("");
	m_strQuery = _T("");
}
	
CuDataTupleSession::CuDataTupleSession(
	LPCTSTR lpszID,
	LPCTSTR lpszName, 
	LPCTSTR lpszTTY,
	LPCTSTR lpszDatabase,
	LPCTSTR lpszState,
	LPCTSTR lpszFacil,
	LPCTSTR lpszQuery)
{
	m_strID = lpszID;
	m_strName = lpszName;
	m_strTTY = lpszTTY;
	m_strDatabase = lpszDatabase;
	m_strState = lpszState;
	m_strFacil = lpszFacil;
	m_strQuery = lpszQuery;
}

void CuDataTupleSession::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strID;
		ar << m_strName;
		ar << m_strTTY;
		ar << m_strDatabase;
		ar << m_strState;
		ar << m_strFacil;
		ar << m_strQuery;
	}
	else
	{
		ar >> m_strID;
		ar >> m_strName;
		ar >> m_strTTY;
		ar >> m_strDatabase;
		ar >> m_strState;
		ar >> m_strFacil;
		ar >> m_strQuery;
	}
}

CuIpmPropertyDataPageSessions::CuIpmPropertyDataPageSessions()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageSessions"))
{

}

CuIpmPropertyDataPageSessions::~CuIpmPropertyDataPageSessions()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageSessions::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageSessions::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}


//
// PAGE: Transactions
//		 tuple = (ExternalTXID, Database, Session, Status, Write, Split)
//		 It must containt the list of tuples.
// *********************************************************************
IMPLEMENT_SERIAL (CuDataTupleTransaction, CObject, 1)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageTransactions, CuIpmPropertyData, 1)

CuDataTupleTransaction::CuDataTupleTransaction()
{
	m_strExternalTXID = _T("");
	m_strDatabase = _T("");
	m_strSession = _T("");
	m_strStatus = _T("");
	m_strWrite = _T("");
	m_strSplit = _T("");
}

CuDataTupleTransaction::CuDataTupleTransaction(
	LPCTSTR lpszExternalTXID,
	LPCTSTR lpszDatabase, 
	LPCTSTR lpszSession,
	LPCTSTR lpszStatus,
	LPCTSTR lpszWrite,
	LPCTSTR lpszSplit)
{
	m_strExternalTXID = lpszExternalTXID;
	m_strDatabase = lpszDatabase;
	m_strSession = lpszSession;
	m_strStatus = lpszStatus;
	m_strWrite = lpszWrite;
	m_strSplit = lpszSplit;
}

void CuDataTupleTransaction::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strExternalTXID;
		ar << m_strDatabase;
		ar << m_strSession;
		ar << m_strStatus;
		ar << m_strWrite;
		ar << m_strSplit;
	}
	else
	{
		ar >> m_strExternalTXID;
		ar >> m_strDatabase;
		ar >> m_strSession;
		ar >> m_strStatus;
		ar >> m_strWrite;
		ar >> m_strSplit;
	}
}

CuIpmPropertyDataPageTransactions::CuIpmPropertyDataPageTransactions()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageTransactions"))
{

}

CuIpmPropertyDataPageTransactions::~CuIpmPropertyDataPageTransactions()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageTransactions::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageTransactions::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}



//
// SUMMARY: LockInfo
// *****************
IMPLEMENT_SERIAL (CuIpmPropertyDataSummaryLockInfo, CuIpmPropertyData, 1)

CuIpmPropertyDataSummaryLockInfo::CuIpmPropertyDataSummaryLockInfo()
	:CuIpmPropertyData(_T("CuIpmPropertyDataSummaryLockInfo"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}

CuIpmPropertyDataSummaryLockInfo::CuIpmPropertyDataSummaryLockInfo(LPLOCKINFOSUMMARY pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataSummaryLockInfo"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}

void CuIpmPropertyDataSummaryLockInfo::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)(LPARAM)&m_dlgData);
}

void CuIpmPropertyDataSummaryLockInfo::Serialize (CArchive& ar)
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
// SUMMARY: LogInfo
// ****************
IMPLEMENT_SERIAL (CuIpmPropertyDataSummaryLogInfo, CuIpmPropertyData, 2)

CuIpmPropertyDataSummaryLogInfo::CuIpmPropertyDataSummaryLogInfo()
	:CuIpmPropertyData(_T("CuIpmPropertyDataSummaryLogInfo"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
}

CuIpmPropertyDataSummaryLogInfo::CuIpmPropertyDataSummaryLogInfo(LPLOGINFOSUMMARY pData)
	:CuIpmPropertyData(_T("CuIpmPropertyDataSummaryLogInfo"))
{
	memset (&m_dlgData, 0, sizeof (m_dlgData));
	memcpy (&m_dlgData, pData, sizeof (m_dlgData));
}

void CuIpmPropertyDataSummaryLogInfo::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_dlgData);
}

void CuIpmPropertyDataSummaryLogInfo::Serialize (CArchive& ar)
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
// Replication Data
// *****************************************************************




//
// SERVER::Startup Setting:
// ************************
IMPLEMENT_SERIAL (CaReplicationServerDataPageStartupSetting, CuIpmPropertyData, 1)

CaReplicationServerDataPageStartupSetting::CaReplicationServerDataPageStartupSetting()
	:CuIpmPropertyData(_T("CaReplicationServerDataPageStartupSetting"))
{
}

void CaReplicationServerDataPageStartupSetting::Serialize (CArchive& ar)
{
	m_listItems.Serialize (ar);
	m_cxHeader.Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_scrollPos;
	}
	else
	{
		ar >> m_scrollPos;
	}
}

void CaReplicationServerDataPageStartupSetting::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listItems);
}


//
// SERVER::Raise Event:
// ********************
IMPLEMENT_SERIAL (CaReplicationServerDataPageRaiseEvent, CuIpmPropertyData, 1)
CaReplicationServerDataPageRaiseEvent::CaReplicationServerDataPageRaiseEvent()
	:CuIpmPropertyData(_T("CaReplicationServerDataPageRaiseEvent"))
{

}

CaReplicationServerDataPageRaiseEvent::~CaReplicationServerDataPageRaiseEvent()
{
	if (!m_listEvent.IsEmpty())
	{
		m_listEvent.RemoveAll();
	}
}

void CaReplicationServerDataPageRaiseEvent::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CaReplicationServerDataPageRaiseEvent::Serialize (CArchive& ar)
{
	m_listEvent.Serialize (ar);
	m_cxHeader.Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_scrollPos;
	}
	else
	{
		ar >> m_scrollPos;
	}
}


//
// SERVER::Assignment:
// *******************
IMPLEMENT_SERIAL (CaReplicationServerDataPageAssignment, CuIpmPropertyData, 1)
CaReplicationServerDataPageAssignment::CaReplicationServerDataPageAssignment()
	:CuIpmPropertyData(_T("CaReplicationServerDataPageAssignment"))
{
}

CaReplicationServerDataPageAssignment::~CaReplicationServerDataPageAssignment()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CaReplicationServerDataPageAssignment::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CaReplicationServerDataPageAssignment::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
	m_cxHeader.Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_scrollPos;
	}
	else
	{
		ar >> m_scrollPos;
	}
}


//
// SERVER::Status:
// *******************
IMPLEMENT_SERIAL (CaReplicationServerDataPageStatus, CuIpmPropertyData, 1)
CaReplicationServerDataPageStatus::CaReplicationServerDataPageStatus()
	:CuIpmPropertyData(_T("CaReplicationServerDataPageStatus"))
{
	memset (&m_dlgData, 0, sizeof(m_dlgData));
	m_strStartLogFile = _T("");
}

CaReplicationServerDataPageStatus::~CaReplicationServerDataPageStatus()
{

}

void CaReplicationServerDataPageStatus::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strStartLogFile;
		ar.Write (&m_dlgData, sizeof (m_dlgData)); 
	}
	else
	{
		ar >> m_strStartLogFile;
		ar.Read (&m_dlgData, sizeof (m_dlgData)); 
	}
}

void CaReplicationServerDataPageStatus::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

//
// REPLICATION STATIC::Activity:
// *****************************
IMPLEMENT_SERIAL (CaReplicationStaticDataPageActivity, CuIpmPropertyData, 1)
CaReplicationStaticDataPageActivity::CaReplicationStaticDataPageActivity()
	:CuIpmPropertyData(_T("CaReplicationStaticDataPageActivity"))
{
	m_nCurrentPage = 0;
	m_strInputQueue = _T("");
	m_strDistributionQueue = _T("");
	m_strStartingTime = _T("");
	m_cxSummary.SetSize (6);
	m_cxDatabaseOutgoing.SetSize (6);
	m_cxDatabaseIncoming.SetSize (6);
	m_cxDatabaseTotal.SetSize (6);
	m_cxTableOutgoing.SetSize (6);
	m_cxTableIncoming.SetSize (6);
	m_cxTableTotal.SetSize (6);
}

CaReplicationStaticDataPageActivity::~CaReplicationStaticDataPageActivity()
{
	while (!m_listSummary.IsEmpty())
		delete m_listSummary.RemoveHead();
	while (!m_listDatabaseOutgoing.IsEmpty())
		delete m_listDatabaseOutgoing.RemoveHead();
	while (!m_listDatabaseIncoming.IsEmpty())
		delete m_listDatabaseIncoming.RemoveHead();
	while (!m_listDatabaseTotal.IsEmpty())
		delete m_listDatabaseTotal.RemoveHead();
	while (!m_listTableOutgoing.IsEmpty())
		delete m_listTableOutgoing.RemoveHead();
	while (!m_listTableIncoming.IsEmpty())
		delete m_listTableIncoming.RemoveHead();
	while (!m_listTableTotal.IsEmpty())
		delete m_listTableTotal.RemoveHead();
}

void CaReplicationStaticDataPageActivity::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CaReplicationStaticDataPageActivity::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_nCurrentPage;
		ar << m_strInputQueue;
		ar << m_strDistributionQueue;
		ar << m_strStartingTime;
		ar << m_scrollSummary;
		ar << m_scrollDatabaseOutgoing;
		ar << m_scrollDatabaseIncoming;
		ar << m_scrollDatabaseTotal;
		ar << m_scrollTableOutgoing;
		ar << m_scrollTableIncoming;
		ar << m_scrollTableTotal;
	}
	else
	{
		ar >> m_nCurrentPage;
		ar >> m_strInputQueue;
		ar >> m_strDistributionQueue;
		ar >> m_strStartingTime;
		ar >> m_scrollSummary;
		ar >> m_scrollDatabaseOutgoing;
		ar >> m_scrollDatabaseIncoming;
		ar >> m_scrollDatabaseTotal;
		ar >> m_scrollTableOutgoing;
		ar >> m_scrollTableIncoming;
		ar >> m_scrollTableTotal;
	}
	m_listSummary.Serialize (ar);
	m_listDatabaseOutgoing.Serialize (ar);
	m_listDatabaseIncoming.Serialize (ar);
	m_listDatabaseTotal.Serialize (ar);
	
	m_listTableOutgoing.Serialize (ar);
	m_listTableIncoming.Serialize (ar);
	m_listTableTotal.Serialize (ar);

	m_cxSummary.Serialize (ar);
	m_cxDatabaseOutgoing.Serialize (ar);
	m_cxDatabaseIncoming.Serialize (ar);
	m_cxDatabaseTotal.Serialize (ar);
	m_cxTableOutgoing.Serialize (ar);
	m_cxTableIncoming.Serialize (ar);
	m_cxTableTotal.Serialize (ar);
}


//
// REPLICATION STATIC::Integrity:
// ******************************
IMPLEMENT_SERIAL (CaReplicationStaticDataPageIntegrity, CuIpmPropertyData, 2)
CaReplicationStaticDataPageIntegrity::CaReplicationStaticDataPageIntegrity()
	:CuIpmPropertyData(_T("CaReplicationStaticDataPageIntegrity"))
{
	// tables combo
	m_RegTableList.ClearList();
	m_strComboTable = _T("");

	// cdds combo
	// nothing to do for m_CDDSList;
	/* FINIR : itemdata */
	m_strComboCdds = _T("");

	// Databases combos
	// nothing to do for m_DbList;
	/* FINIR : itemdata */
	m_strComboDatabase1 = _T("");
	m_strComboDatabase2 = _T("");

	// edit fields
	m_strTimeBegin = _T("");
	m_strTimeEnd = _T("");
	m_strResult = _T("");
}

CaReplicationStaticDataPageIntegrity::~CaReplicationStaticDataPageIntegrity()
{
}

void CaReplicationStaticDataPageIntegrity::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CaReplicationStaticDataPageIntegrity::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strComboTable;
		/* FINIR : ar << itemdata */
		ar << m_strComboCdds;
		/* FINIR : ar << itemdata */
		ar << m_strComboDatabase1;
		ar << m_strComboDatabase2;
		ar << m_strTimeBegin;
		ar << m_strTimeEnd;
		ar << m_strResult;
	}
	else
	{
		ar >> m_strComboTable;
		/* FINIR : ar >> itemdata */
		ar >> m_strComboCdds;
		/* FINIR : ar >> itemdata */
		ar >> m_strComboDatabase1;
		ar >> m_strComboDatabase2;
		ar >> m_strTimeBegin;
		ar >> m_strTimeEnd;
		ar >> m_strResult;
	}

	m_RegTableList.Serialize (ar);
	m_CDDSList.Serialize (ar);
	m_DbList1.Serialize (ar);
	m_DbList2.Serialize (ar);
	m_ColList.Serialize (ar);
}

//
// REPLICATION STATIC::Server:
// ***************************
IMPLEMENT_SERIAL (CaReplicationStaticDataPageServer, CuIpmPropertyData, 1)
CaReplicationStaticDataPageServer::CaReplicationStaticDataPageServer()
	:CuIpmPropertyData(_T("CaReplicationStaticDataPageServer"))
{
}

CaReplicationStaticDataPageServer::~CaReplicationStaticDataPageServer()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CaReplicationStaticDataPageServer::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CaReplicationStaticDataPageServer::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
	m_cxHeader.Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_scrollPos;
	}
	else
	{
		ar >> m_scrollPos;
	}
}


//
// REPLICATION STATIC::Distrib:
// ****************************
IMPLEMENT_SERIAL (CaReplicationStaticDataPageDistrib, CuIpmPropertyData, 1)
CaReplicationStaticDataPageDistrib::CaReplicationStaticDataPageDistrib()
	:CuIpmPropertyData(_T("CaReplicationStaticDataPageDistrib"))
{
	m_strText = _T("");
}

CaReplicationStaticDataPageDistrib::~CaReplicationStaticDataPageDistrib()
{
}

void CaReplicationStaticDataPageDistrib::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)(LPCTSTR)m_strText);
}

void CaReplicationStaticDataPageDistrib::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strText;
	}
	else
	{
		ar >> m_strText;
	}
}


//
// REPLICATION STATIC::Collision:
// ******************************
IMPLEMENT_SERIAL (CaReplicationStaticDataPageCollision, CuIpmPropertyData, 1)
CaReplicationStaticDataPageCollision::CaReplicationStaticDataPageCollision()
	:CuIpmPropertyData(_T("CaReplicationStaticDataPageCollision"))
{
	m_pData = NULL;
}

CaReplicationStaticDataPageCollision::~CaReplicationStaticDataPageCollision()
{
}

void CaReplicationStaticDataPageCollision::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CaReplicationStaticDataPageCollision::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_pData;
	}
	else
	{
		ar >> m_pData;
	}
}

//
// PAGE: Static SERVERS
// *****************************************************************
IMPLEMENT_SERIAL (CuDataTupleServer, CObject, 2)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageServers, CuIpmPropertyData, 2)

CuDataTupleServer::CuDataTupleServer()
{
	m_strName	= _T("");
}

CuDataTupleServer::CuDataTupleServer(
	LPCTSTR lpszName,
	LPCTSTR lpszListenAddress,
	LPCTSTR lpszClass,
	LPCTSTR lpszDbList,
	int     imageIndex)
{
	m_strName   = lpszName;
	m_strListenAddress = lpszListenAddress;
	m_strClass  = lpszClass;
	m_strDbList = lpszDbList;
	m_imageIndex = imageIndex;
}

void CuDataTupleServer::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strName;
		ar << m_strListenAddress;
		ar << m_strClass;
		ar << m_strDbList;
		ar << m_imageIndex;
	}
	else
	{
		ar >> m_strName;
		ar >> m_strListenAddress;
		ar >> m_strClass;
		ar >> m_strDbList;
		ar >> m_imageIndex;
	}
}

CuIpmPropertyDataPageServers::CuIpmPropertyDataPageServers()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageServers"))
{

}

CuIpmPropertyDataPageServers::~CuIpmPropertyDataPageServers()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageServers::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageServers::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}

//
// PAGE: Static ACTIVE USERS
// *****************************************************************
IMPLEMENT_SERIAL (CuDataTupleActiveuser, CObject, 1)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageActiveusers, CuIpmPropertyData, 1)

CuDataTupleActiveuser::CuDataTupleActiveuser()
{
	m_strName = _T("");
}

CuDataTupleActiveuser::CuDataTupleActiveuser(LPCTSTR lpszName)
{
	m_strName = lpszName;
}

void CuDataTupleActiveuser::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strName;
	}
	else
	{
		ar >> m_strName;
	}
}

CuIpmPropertyDataPageActiveusers::CuIpmPropertyDataPageActiveusers()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageActiveusers"))
{

}

CuIpmPropertyDataPageActiveusers::~CuIpmPropertyDataPageActiveusers()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageActiveusers::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageActiveusers::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}

//
// PAGE: Static DATABASES
// *****************************************************************
IMPLEMENT_SERIAL (CuDataTupleDatabase, CObject, 1)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageStaticDatabases, CuIpmPropertyData, 1)

CuDataTupleDatabase::CuDataTupleDatabase()
{
	m_strName = _T("");
	m_imageIndex = -1;
}

CuDataTupleDatabase::CuDataTupleDatabase(LPCTSTR lpszName, int imageIndex)
{
	m_strName	 = lpszName;
	m_imageIndex = imageIndex;
}

void CuDataTupleDatabase::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strName;
		ar << m_imageIndex;
	}
	else
	{
		ar >> m_strName;
		ar >> m_imageIndex;
	}
}

CuIpmPropertyDataPageStaticDatabases::CuIpmPropertyDataPageStaticDatabases()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageStaticDatabases"))
{

}

CuIpmPropertyDataPageStaticDatabases::~CuIpmPropertyDataPageStaticDatabases()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageStaticDatabases::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageStaticDatabases::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}

//
// PAGE: Static REPLICATION
// *****************************************************************
IMPLEMENT_SERIAL (CuDataTupleReplication, CObject, 1)
IMPLEMENT_SERIAL (CuIpmPropertyDataPageStaticReplications, CuIpmPropertyData, 1)

CuDataTupleReplication::CuDataTupleReplication()
{
	m_strName = _T("");
	m_imageIndex = -1;
}
	
CuDataTupleReplication::CuDataTupleReplication(LPCTSTR lpszName, int imageIndex)
{
	m_strName  = lpszName;
	m_imageIndex = imageIndex;
}

void CuDataTupleReplication::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strName;
		ar << m_imageIndex;
	}
	else
	{
		ar >> m_strName;
		ar >> m_imageIndex;
	}
}

CuIpmPropertyDataPageStaticReplications::CuIpmPropertyDataPageStaticReplications()
	:CuIpmPropertyData(_T("CuIpmPropertyDataPageStaticReplications"))
{

}

CuIpmPropertyDataPageStaticReplications::~CuIpmPropertyDataPageStaticReplications()
{
	while (!m_listTuple.IsEmpty())
	{
		delete m_listTuple.RemoveHead();
	}
}

void CuIpmPropertyDataPageStaticReplications::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}

void CuIpmPropertyDataPageStaticReplications::Serialize (CArchive& ar)
{
	m_listTuple.Serialize (ar);
}

