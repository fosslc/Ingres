/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : cbfitem.h, header file 
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut 
**               Blatte Emannuel 
**    Purpose  : Item data to store information about item (Components) in the List 
**               Control of the Lef-pane of Tab Configure.
**
** History:
** xx-Sep-1997 (uk$so01)
**    Created.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 07-Jul-2003 (schph01)
**    (bug 106858) consistently with CBF change 455479
**    Add m_bDisplayAllTabs variable and new constructor with parameter
**    in CuSECUREItemData class 
** 12-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 06-May-2007 (drivi01)
**    SIR #118462
**    Add Item data for DB2 UDB Gateway.  Previously it used Oracle 
**    item data.  
*/
#if !defined (CBFLISTVIEWITEM_HEADER)
#define CBFLISTVIEWITEM_HEADER
#include "ll.h"
#include "cbfdisp.h"

class CConfLeftDlg;
class CuCbfListViewItem : public CObject
{
	DECLARE_SERIAL (CuCbfListViewItem)
public:
	CuCbfListViewItem();
	virtual ~CuCbfListViewItem();
	virtual CuCbfListViewItem* Copy();
	void SetComponentInfo (LPCOMPONENTINFO lpComponent){memcpy (&m_componentInfo, lpComponent, sizeof(m_componentInfo));}
	void SetConfLeftDlg(CConfLeftDlg* pLeftDlg) { m_pLeftDlg = pLeftDlg; }


	//
	// Data.
	COMPONENTINFO m_componentInfo;
protected:
	CuPageInformation*  m_pPageInfo;
	CConfLeftDlg *m_pLeftDlg;
  int m_ActivationCount;

	//
	// Operations
public:
	void Serialize (CArchive& ar);
	virtual CuPageInformation*   GetPageInformation (){return m_pPageInfo;};

	// Actions on low level when is activated or deactivated
	// default behaviour in this base class:
	// call InitGenericPane() on activation,
	// and call OnMainComponentExit() on deactivation
	// Derive these methods in classes that have custom behaviour only!
	virtual BOOL LowLevelActivation();
	virtual BOOL LowLevelDeactivation();
  // Fix Emb Nov. 19, 97: manage activation/deactivation counter
	BOOL LowLevelActivationWithCheck();
	BOOL LowLevelDeactivationWithCheck();
};


//
// Indexes definition for image associated with each item data,
// used by CConfLeftDlg class - see image list IDB_BITMAP1

#define INDEX_IMGLIST_NAME         0
#define INDEX_IMGLIST_DBMS         1
#define INDEX_IMGLIST_NET          2
#define INDEX_IMGLIST_ICE          3
#define INDEX_IMGLIST_BRIDGE       4
#define INDEX_IMGLIST_STAR         5
#define INDEX_IMGLIST_SECURE       6
#define INDEX_IMGLIST_LOCK         7
#define INDEX_IMGLIST_LOG          8
#define INDEX_IMGLIST_LOGFILE      9
#define INDEX_IMGLIST_RECOVER     10
#define INDEX_IMGLIST_ARCHIVE     11
#define INDEX_IMGLIST_RMCMD       12
#define INDEX_IMGLIST_GW_ORACLE   13
#define INDEX_IMGLIST_GW_INFORMIX 13
#define INDEX_IMGLIST_GW_SYBASE   13
#define INDEX_IMGLIST_GW_MSSQL    13
#define INDEX_IMGLIST_GW_ODBC     13
#define INDEX_IMGLIST_JDBC        14
#define INDEX_IMGLIST_DASVR       INDEX_IMGLIST_JDBC

//
// NAME Item data
// --------------

class CuNAMEItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuNAMEItemData)

public:
	CuNAMEItemData();
	virtual ~CuNAMEItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

};


//
// DBMS Item data
// --------------

class CuDBMSItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuDBMSItemData)

public:
	CuDBMSItemData();
	virtual ~CuDBMSItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

	// Special code on activation/deactivation
	virtual BOOL LowLevelActivation();
	virtual BOOL LowLevelDeactivation();
	
	// for databases management
	// because of calls to VCBFll_dblist_init() and VCBFOnDBListExit()
	BOOL    m_bDbListModified;
	char*   m_initialDbList;
	CString m_resultDbList;
	BOOL    m_bMustFillListbox;
};


//
// ICE Item data
// --------------

class CuICEItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuICEItemData)

public:
	CuICEItemData();
	virtual ~CuICEItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

};

//
// NET Item data
// --------------

class CuNETItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuNETItemData)

public:
	CuNETItemData();
	virtual ~CuNETItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

};

//
// JDBC Item data
// --------------

class CuJDBCItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuJDBCItemData)

public:
	CuJDBCItemData();
	virtual ~CuJDBCItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

};

//
// DASVR Item data
// --------------
class CuDASVRItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuDASVRItemData)

public:
	CuDASVRItemData();
	virtual ~CuDASVRItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

};


//
// BRIDGE Item data
// --------------

class CuBRIDGEItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuBRIDGEItemData)

public:
	CuBRIDGEItemData();
	virtual ~CuBRIDGEItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

	// Special code on activation/deactivation
	virtual BOOL LowLevelActivation();
	virtual BOOL LowLevelDeactivation();
};


//
// STAR Item data
// --------------

class CuSTARItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuSTARItemData)

public:
	CuSTARItemData();
	virtual ~CuSTARItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

};

//
// SECURE Item data
// --------------

class CuSECUREItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuSECUREItemData)

public:
	CuSECUREItemData();
	CuSECUREItemData(BOOL bDisp);

	virtual ~CuSECUREItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

	// Special code on activation/deactivation
	virtual BOOL LowLevelActivation();
	virtual BOOL LowLevelDeactivation();

private:
	// variable indicating in which subset of property pages we are
	// Acceptable values: GEN_FORM_SECURE_SECURE for 2 left property pages group,
	// or GEN_FORM_SECURE_C2 for 3 right property pages group
	int m_ispecialtype;
	/* TRUE display all tabs */
	/* FALSE display only "System" and "Mechanisms" tabs */
	BOOL m_bDisplayAllTabs;
public:
  int GetSpecialType() {return m_ispecialtype; }
  void SetSpecialType(int type);

  int  GetDisplayTabs() {return m_bDisplayAllTabs; }
  void SetDisplayTabs(BOOL bDisp){m_bDisplayAllTabs = bDisp;}
};

//
// LOCK Item data
// --------------

class CuLOCKItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuLOCKItemData)

public:
	CuLOCKItemData();
	virtual ~CuLOCKItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

};

//
// LOG Item data
// --------------

class CuLOGItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuLOGItemData)

public:
	CuLOGItemData();
	virtual ~CuLOGItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

};

//
// LOGFILE Item data
// --------------

class CuLOGFILEItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuLOGFILEItemData)

public:
	TRANSACTINFO  m_transactionInfo;
	CuLOGFILEItemData();
	virtual ~CuLOGFILEItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

	// Special code on activation/deactivation
	virtual BOOL LowLevelActivation();
	virtual BOOL LowLevelDeactivation();
};

//
// RECOVER Item data
// --------------

class CuRECOVERItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuRECOVERItemData)

public:
	CuRECOVERItemData();
	virtual ~CuRECOVERItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

	// Special code on activation/deactivation :
	// nothing to call on low-level side
	// virtual BOOL LowLevelActivation()   { return TRUE; }
	// virtual BOOL LowLevelDeactivation() { return TRUE; }
};

//
// ARCHIVE Item data
// --------------

class CuARCHIVEItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuARCHIVEItemData)

public:
	CuARCHIVEItemData();
	virtual ~CuARCHIVEItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

	// Special code on activation/deactivation :
	// nothing to call on low-level side
	virtual BOOL LowLevelActivation()   { return TRUE; }
	virtual BOOL LowLevelDeactivation() { return TRUE; }
};


//
// RMCMD Item data
// --------------

class CuRMCMDItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuRMCMDItemData)

public:
	CuRMCMDItemData();
	virtual ~CuRMCMDItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }

	// Special code on activation/deactivation :
	// nothing to call on low-level side
	virtual BOOL LowLevelActivation()   { return TRUE; }
	virtual BOOL LowLevelDeactivation() { return TRUE; }
};

//
// GW_ORACLE Item data
// -------------------

class CuGW_ORACLEItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuGW_ORACLEItemData)

public:
	CuGW_ORACLEItemData();
	virtual ~CuGW_ORACLEItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }
};


//
// GW_DB2UDB Item data
// -------------------

class CuGW_DB2UDBItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuGW_DB2UDBItemData)

public:
	CuGW_DB2UDBItemData();
	virtual ~CuGW_DB2UDBItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }
};


//
// GW_INFORMIX Item data
// -------------------

class CuGW_INFORMIXItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuGW_INFORMIXItemData)

public:
	CuGW_INFORMIXItemData();
	virtual ~CuGW_INFORMIXItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }
};

//
// GW_SYBASE Item data
// -------------------

class CuGW_SYBASEItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuGW_SYBASEItemData)

public:
	CuGW_SYBASEItemData();
	virtual ~CuGW_SYBASEItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }
};

//
// GW_MSSQL Item data
// -------------------

class CuGW_MSSQLItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuGW_MSSQLItemData)

public:
	CuGW_MSSQLItemData();
	virtual ~CuGW_MSSQLItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }
};

//
// GW_ODBC Item data
// -------------------

class CuGW_ODBCItemData : public CuCbfListViewItem
{
	DECLARE_SERIAL (CuGW_ODBCItemData)

public:
	CuGW_ODBCItemData();
	virtual ~CuGW_ODBCItemData(){};
	virtual CuCbfListViewItem* Copy();

	CuPageInformation* GetPageInformation ();
	void Serialize (CArchive& ar) { CuCbfListViewItem::Serialize(ar); }
};

#endif