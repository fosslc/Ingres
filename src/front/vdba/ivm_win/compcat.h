/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : compcat.h , Header File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation Ingres Visual Manager Data
**
** History:
**
** 13-Jan-1999 (uk$so01)
**    Created
** 18-Jan-2001 (uk$so01)
**    SIR #103727 (Make IVM support JDBC Server)
**    Add the tree item data 'CaItemDataJdbc'
** 14-Aug-2001 (uk$so01)
**    SIR #105383., Remove meber CaItemDataRecovery::MakeActiveTab() and
**    CaItemDataArchiver::MakeActiveTab()
** 03-Oct-2002 (noifr01)
**    (SIR 107587) have IVM manage DB2UDB gateway (mapping the GUI to
**    oracle gateway code until this is moved to generic management)
** 19-Dec-2002 (schph01)
**    bug 109330 Verify that an instance exist to update the "Stop" option
**    menu
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
**/

#if !defined(COMPCATEGORY_HEADER)
#define COMPCATEGORY_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "compdata.h"

//
// BRIDGE Item data
// --------------
class CaItemDataBridge : public CaTreeComponentFolderItemData
{
public:
	CaItemDataBridge();
	virtual ~CaItemDataBridge(){};
	virtual int GetInstaneImageRead(){return IM_BRIDGE_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_BRIDGE_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}
	virtual CString GetStartStopString(BOOL bStart = TRUE);

	CaPageInformation* GetPageInformation ();
};




//
// DBMS Item data
// --------------
class CaItemDataDbms : public CaTreeComponentFolderItemData
{
public:
	CaItemDataDbms();
	virtual ~CaItemDataDbms(){};
	virtual int GetInstaneImageRead(){return IM_DBMS_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_DBMS_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}
	virtual CString GetStartStopString(BOOL bStart = TRUE);

	CaPageInformation* GetPageInformation ();

	
};


//
// NET Item data
// --------------
class CaItemDataNet : public CaTreeComponentFolderItemData
{
public:
	CaItemDataNet();
	virtual ~CaItemDataNet(){};
	virtual int GetInstaneImageRead(){return IM_NET_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_NET_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}
	virtual CString GetStartStopString(BOOL bStart = TRUE);

	CaPageInformation* GetPageInformation ();

};




//
// STAR Item data
// --------------
class CaItemDataStar : public CaTreeComponentFolderItemData
{
public:
	CaItemDataStar();
	virtual ~CaItemDataStar(){};
	virtual int GetInstaneImageRead(){return IM_STAR_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_STAR_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}
	virtual CString GetStartStopString(BOOL bStart = TRUE);

	CaPageInformation* GetPageInformation ();

};

//
// JDBC Item data
// --------------
class CaItemDataJdbc : public CaTreeComponentFolderItemData
{
public:
	CaItemDataJdbc();
	virtual ~CaItemDataJdbc(){};
	virtual int GetInstaneImageRead(){return IM_JDBC_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_JDBC_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}
	virtual CString GetStartStopString(BOOL bStart = TRUE);

	CaPageInformation* GetPageInformation ();
};

//
// DASSVR Item data
// --------------
class CaItemDataDasvr : public CaTreeComponentFolderItemData
{
public:
	CaItemDataDasvr();
	virtual ~CaItemDataDasvr(){};
	virtual int GetInstaneImageRead(){return IM_DASVR_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_DASVR_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}
	virtual CString GetStartStopString(BOOL bStart = TRUE);

	CaPageInformation* GetPageInformation ();
};

//
// ICE Item data
// --------------
class CaItemDataIce : public CaTreeComponentItemData
{
public:
	CaItemDataIce();
	virtual ~CaItemDataIce(){};
	virtual int GetInstaneImageRead(){return IM_ICE_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_ICE_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}

	CaPageInformation* GetPageInformation ();

};



//
// NAME Item data
// --------------
class CaItemDataName : public CaTreeComponentItemData
{
public:
	CaItemDataName();
	virtual ~CaItemDataName(){};
	virtual int GetInstaneImageRead(){return IM_NAME_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_NAME_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}

	CaPageInformation* GetPageInformation ();

};


//
// SECURE Item data
// --------------
class CaItemDataSecurity : public CaTreeComponentItemData
{
public:
	CaItemDataSecurity();
	virtual ~CaItemDataSecurity(){};
	virtual int GetInstaneImageRead(){return IM_SECURITY_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_SECURITY_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);

	CaPageInformation* GetPageInformation ();
};

//
// LOCK Item data
// --------------
class CaItemDataLocking : public CaTreeComponentItemData
{
public:
	CaItemDataLocking();
	virtual ~CaItemDataLocking(){};
	virtual int GetInstaneImageRead(){return IM_LOCKING_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_LOCKING_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);

	CaPageInformation* GetPageInformation ();
};

//
// LOG Item data
// --------------
class CaItemDataLogging : public CaTreeComponentItemData
{
public:
	CaItemDataLogging();
	virtual ~CaItemDataLogging(){};
	virtual int GetInstaneImageRead(){return IM_LOGGING_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_LOGGING_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);

	CaPageInformation* GetPageInformation ();
};

//
// LOGFILE Item data
// --------------

class CaItemDataLogFile : public CaTreeComponentItemData
{
public:
	CaItemDataLogFile();
	virtual ~CaItemDataLogFile(){};
	virtual int GetInstaneImageRead(){return IM_TRANSACTION_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_TRANSACTION_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);

	CaPageInformation* GetPageInformation ();
};

//
// RECOVER Item data
// --------------
class CaItemDataRecovery : public CaTreeComponentItemData
{
public:
	CaItemDataRecovery();
	virtual ~CaItemDataRecovery(){};
	virtual int GetInstaneImageRead(){return IM_RECOVERY_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_RECOVERY_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}

	CaPageInformation* GetPageInformation ();
};

//
// ARCHIVE Item data
// --------------
class CaItemDataArchiver : public CaTreeComponentItemData
{
public:
	CaItemDataArchiver();
	virtual ~CaItemDataArchiver(){};
	virtual int GetInstaneImageRead(){return IM_ARCHIVER_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_ARCHIVER_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}

	CaPageInformation* GetPageInformation ();
};



//
// OpenIngres DESKTOP DBMS Item data
// ---------------------------------
class CaItemDataDbmsOidt : public CaTreeComponentFolderItemData
{
public:
	CaItemDataDbmsOidt();
	virtual ~CaItemDataDbmsOidt(){};
	virtual int GetInstaneImageRead(){return IM_DBMS_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_DBMS_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);

	CaPageInformation* GetPageInformation ();
};


//
// RMCMD Item data
// --------------
class CaItemDataRmcmd : public CaTreeComponentItemData
{
public:
	CaItemDataRmcmd();
	virtual ~CaItemDataRmcmd(){};
	virtual int GetInstaneImageRead(){return IM_RMCMD_R_INSTANCE;}
	virtual int GetInstaneImageNotRead(){return IM_RMCMD_N_INSTANCE;}
	virtual void UpdateIcon(CTreeCtrl* pTree);
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}

	CaPageInformation* GetPageInformation ();
};

//
// GW_ORACLE Item data
// -------------------
class CaItemDataOracle : public CaTreeComponentGatewayItemData
{
public:
	CaItemDataOracle();
	virtual void SetStartStopArg(LPCTSTR lpstartstoparg);
	virtual ~CaItemDataOracle(){};
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}

	CaPageInformation* GetPageInformation ();
};

//
// GW_INFORMIX Item data
// -------------------
class CaItemDataInformix : public CaTreeComponentGatewayItemData
{
public:
	CaItemDataInformix();
	virtual ~CaItemDataInformix(){};
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}
	CaPageInformation* GetPageInformation ();
};

//
// GW_SYBASE Item data
// -------------------
class CaItemDataSybase : public CaTreeComponentGatewayItemData
{
public:
	CaItemDataSybase();
	virtual ~CaItemDataSybase(){};
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}

	CaPageInformation* GetPageInformation ();
};

//
// GW_MSSQL Item data
// -------------------
class CaItemDataMsSql : public CaTreeComponentGatewayItemData
{
public:
	CaItemDataMsSql();
	virtual ~CaItemDataMsSql(){};
	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}

	CaPageInformation* GetPageInformation ();
};

//
// GW_ODBC Item data
// -------------------
class CaItemDataOdbc : public CaTreeComponentGatewayItemData
{
public:
	CaItemDataOdbc();
	virtual ~CaItemDataOdbc(){};

	virtual BOOL IsStartEnabled(){return TRUE;}
	virtual BOOL IsStopEnabled (){return ExistInstance();}
	CaPageInformation* GetPageInformation ();
};

#endif // !defined(COMPCATEGORY_HEADER)
