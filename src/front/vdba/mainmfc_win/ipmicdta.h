/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : ipmicdta.h, Header file.                                              //
//                                                                                     //
//                                                                                     //
//    Project  : INGRES II/ Ice Monitoring.                                            //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : The implementation of serialization of IPM Page (modeless dialog)     //
****************************************************************************************/
#ifndef IPMICDTA_HEADER
#define IPMICDTA_HEADER
extern "C"
{
#include "dba.h"
#include "dbaset.h"
#include "monitor.h"
}
#include "ipmprdta.h"

//
// DETAIL: ICE CONNECTED USER
// ***************************************************************
class CaIpmPropertyDataIceDetailConnectedUser: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIceDetailConnectedUser)
public:
	CaIpmPropertyDataIceDetailConnectedUser ();
	CaIpmPropertyDataIceDetailConnectedUser (ICE_CONNECTED_USERDATAMIN* pData);
	virtual ~CaIpmPropertyDataIceDetailConnectedUser(){};

	virtual void NotifyLoad (CWnd* pDlg);
	virtual void Serialize  (CArchive& ar);
	
	ICE_CONNECTED_USERDATAMIN m_dlgData;
};


//
// DETAIL: ICE TRANSACTION
// ***************************************************************
class CaIpmPropertyDataIceDetailTransaction: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIceDetailTransaction)
public:
	CaIpmPropertyDataIceDetailTransaction ();
	CaIpmPropertyDataIceDetailTransaction (ICE_TRANSACTIONDATAMIN* pData);
	virtual ~CaIpmPropertyDataIceDetailTransaction(){};

	virtual void NotifyLoad (CWnd* pDlg);
	virtual void Serialize  (CArchive& ar);
	
	ICE_TRANSACTIONDATAMIN m_dlgData;
};


//
// DETAIL: ICE CURSOR
// ***************************************************************
class CaIpmPropertyDataIceDetailCursor: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIceDetailCursor)
public:
	CaIpmPropertyDataIceDetailCursor ();
	CaIpmPropertyDataIceDetailCursor (ICE_CURSORDATAMIN* pData);
	virtual ~CaIpmPropertyDataIceDetailCursor(){};

	virtual void NotifyLoad (CWnd* pDlg);
	virtual void Serialize  (CArchive& ar);
	
	ICE_CURSORDATAMIN m_dlgData;
};



//
// DETAIL: ICE CACHE PAGE
// ***************************************************************
class CaIpmPropertyDataIceDetailCachePage: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIceDetailCachePage)
public:
	CaIpmPropertyDataIceDetailCachePage ();
	CaIpmPropertyDataIceDetailCachePage (ICE_CACHEINFODATAMIN* pData);
	virtual ~CaIpmPropertyDataIceDetailCachePage(){};

	virtual void NotifyLoad (CWnd* pDlg);
	virtual void Serialize  (CArchive& ar);
	
	ICE_CACHEINFODATAMIN m_dlgData;
};


//
// DETAIL: ICE ACTIVE USER
// ***************************************************************
class CaIpmPropertyDataIceDetailActiveUser: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIceDetailActiveUser)
public:
	CaIpmPropertyDataIceDetailActiveUser ();
	CaIpmPropertyDataIceDetailActiveUser (ICE_ACTIVE_USERDATAMIN* pData);
	virtual ~CaIpmPropertyDataIceDetailActiveUser(){};

	virtual void NotifyLoad (CWnd* pDlg);
	virtual void Serialize  (CArchive& ar);
	
	ICE_ACTIVE_USERDATAMIN m_dlgData;
};



//
// DETAIL: ICE HTTP SERVER CONNECTION
// ***************************************************************
class CaIpmPropertyDataIceDetailHttpServerConnection: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIceDetailHttpServerConnection)
public:
	CaIpmPropertyDataIceDetailHttpServerConnection();
	CaIpmPropertyDataIceDetailHttpServerConnection (SESSIONDATAMIN* pData);
	virtual ~CaIpmPropertyDataIceDetailHttpServerConnection(){}

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	ICE_ACTIVE_USERDATAMIN m_dlgData;
};

//
// DETAIL: ICE DATABASE CONNECTION
// ***************************************************************
class CaIpmPropertyDataIceDetailDatabaseConnection: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIceDetailDatabaseConnection)
public:
	CaIpmPropertyDataIceDetailDatabaseConnection ();
	CaIpmPropertyDataIceDetailDatabaseConnection (ICE_DB_CONNECTIONDATAMIN* pData);
	virtual ~CaIpmPropertyDataIceDetailDatabaseConnection(){};

	virtual void NotifyLoad (CWnd* pDlg);
	virtual void Serialize  (CArchive& ar);
	
	ICE_DB_CONNECTIONDATAMIN m_dlgData;
};




//
// PAGE: ICE CONNECTED USER
// ***************************************************************
class CaIpmPropertyDataIcePageConnectedUser: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIcePageConnectedUser)
public:
	CaIpmPropertyDataIcePageConnectedUser();
	virtual ~CaIpmPropertyDataIcePageConnectedUser();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CPtrList, ICE_CONNECTED_USERDATAMIN*> m_listTuple;
};


//
// PAGE: ICE TRANSACTION
// ***************************************************************
class CaIpmPropertyDataIcePageTransaction: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIcePageTransaction)
public:
	CaIpmPropertyDataIcePageTransaction();
	virtual ~CaIpmPropertyDataIcePageTransaction();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CPtrList, ICE_TRANSACTIONDATAMIN*> m_listTuple;
};


//
// PAGE: ICE CURSOR
// ***************************************************************
class CaIpmPropertyDataIcePageCursor: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIcePageCursor)
public:
	CaIpmPropertyDataIcePageCursor();
	virtual ~CaIpmPropertyDataIcePageCursor();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CPtrList, ICE_CURSORDATAMIN*> m_listTuple;
};


//
// PAGE: ICE CACHE PAGE
// ***************************************************************
class CaIpmPropertyDataIcePageCachePage: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIcePageCachePage)
public:
	CaIpmPropertyDataIcePageCachePage();
	virtual ~CaIpmPropertyDataIcePageCachePage();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CPtrList, ICE_CACHEINFODATAMIN*> m_listTuple;
};




//
// PAGE: ICE ACTIVE USER
// ***************************************************************
class CaIpmPropertyDataIcePageActiveUser: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIcePageActiveUser)
public:
	CaIpmPropertyDataIcePageActiveUser();
	virtual ~CaIpmPropertyDataIcePageActiveUser();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CPtrList, ICE_ACTIVE_USERDATAMIN*> m_listTuple;
};

//
// PAGE: ICE HTTP SERVER CONNECTION
// ***************************************************************
class CaIpmPropertyDataIcePageHttpServerConnection: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIcePageHttpServerConnection)
public:
	CaIpmPropertyDataIcePageHttpServerConnection();
	virtual ~CaIpmPropertyDataIcePageHttpServerConnection();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CPtrList, SESSIONDATAMIN*> m_listTuple;
};

//
// PAGE: ICE DATABASE CONNECTION
// ***************************************************************
class CaIpmPropertyDataIcePageDatabaseConnection: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaIpmPropertyDataIcePageDatabaseConnection)
public:
	CaIpmPropertyDataIcePageDatabaseConnection();
	virtual ~CaIpmPropertyDataIcePageDatabaseConnection();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CPtrList, ICE_DB_CONNECTIONDATAMIN*> m_listTuple;
};



#endif
