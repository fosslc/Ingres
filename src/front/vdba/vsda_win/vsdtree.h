/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdtree.h : main tree of the left pane
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Tree of ingres object (starting from database)
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 22-Apr-2003 (schph01)
**    SIR 107523 Add Sequence Object
** 18-Aug-2004 (uk$so01)
**    BUG #112841 / ISSUE #13623161, Allow user to cancel the Comparison
**    operation.
** 29-Sep-2004 (uk$so01)
**    BUG #113119, Add readlock mode in the session management and
**    optimize the display.
** 21-Oct-2004 (uk$so01)
**    BUG #113280 / ISSUE 13742473 (VDDA should minimize the number of DBMS connections)
*/

#if !defined (VSD_TREE_HEADER)
#define VSD_TREE_HEADER
#include "trfdbase.h"
#include "trfuser.h"
#include "trfgroup.h"
#include "trfrole.h"
#include "trfprofi.h"
#include "trflocat.h"
#include "trfgrant.h"
#include "trfalarm.h"
#include "trfseq.h"
#include "loadsave.h"

//
// Image list IDs
enum 
{
	// Folder:
	IM_VSDTRF_FOLDER_CLOSE_GG = 0,
	IM_VSDTRF_FOLDER_CLOSE_GR,
	IM_VSDTRF_FOLDER_CLOSE_RG,
	IM_VSDTRF_FOLDER_CLOSE_YY,
	IM_VSDTRF_FOLDER_OPEN_GG,
	IM_VSDTRF_FOLDER_OPEN_GR,
	IM_VSDTRF_FOLDER_OPEN_RG,
	IM_VSDTRF_FOLDER_OPEN_YY,
	// Folder with discard mark
	IM_VSDTRFd_FOLDER_CLOSE_GG,
	IM_VSDTRFd_FOLDER_CLOSE_GR,
	IM_VSDTRFd_FOLDER_CLOSE_RG,
	IM_VSDTRFd_FOLDER_CLOSE_YY,
	IM_VSDTRFd_FOLDER_OPEN_GG,
	IM_VSDTRFd_FOLDER_OPEN_GR,
	IM_VSDTRFd_FOLDER_OPEN_RG,
	IM_VSDTRFd_FOLDER_OPEN_YY,
	// Empty image
	IM_VSDTRF_EMPTY,
	// Discard image
	IM_VSDTRFd,
	// Installation
	IM_VSDTRF_INSTALLATION_GG,
	IM_VSDTRF_INSTALLATION_GR,
	IM_VSDTRF_INSTALLATION_RG,
	IM_VSDTRF_INSTALLATION_YY,
	// Installation with discard mark
	IM_VSDTRFd_INSTALLATION_GG,
	IM_VSDTRFd_INSTALLATION_GR,
	IM_VSDTRFd_INSTALLATION_RG,
	IM_VSDTRFd_INSTALLATION_YY,
	// Group 
	IM_VSDTRF_GROUP_GG,
	IM_VSDTRF_GROUP_GR,
	IM_VSDTRF_GROUP_RG,
	IM_VSDTRF_GROUP_YY,
	// Group with discard mark
	IM_VSDTRFd_GROUP_GG,
	IM_VSDTRFd_GROUP_GR,
	IM_VSDTRFd_GROUP_RG,
	IM_VSDTRFd_GROUP_YY,
	// User:
	IM_VSDTRF_USER_GG,
	IM_VSDTRF_USER_GR,
	IM_VSDTRF_USER_RG,
	IM_VSDTRF_USER_YY,
	// User with discard mark:
	IM_VSDTRFd_USER_GG,
	IM_VSDTRFd_USER_GR,
	IM_VSDTRFd_USER_RG,
	IM_VSDTRFd_USER_YY,
	// Role:
	IM_VSDTRF_ROLE_GG,
	IM_VSDTRF_ROLE_GR,
	IM_VSDTRF_ROLE_RG,
	IM_VSDTRF_ROLE_YY,
	// Role with discard mark:
	IM_VSDTRFd_ROLE_GG,
	IM_VSDTRFd_ROLE_GR,
	IM_VSDTRFd_ROLE_RG,
	IM_VSDTRFd_ROLE_YY,
	// Profile:
	IM_VSDTRF_PROFILE_GG,
	IM_VSDTRF_PROFILE_GR,
	IM_VSDTRF_PROFILE_RG,
	IM_VSDTRF_PROFILE_YY,
	// Profile with discard mark:
	IM_VSDTRFd_PROFILE_GG,
	IM_VSDTRFd_PROFILE_GR,
	IM_VSDTRFd_PROFILE_RG,
	IM_VSDTRFd_PROFILE_YY,
	// Location:
	IM_VSDTRF_LOCATION_GG,
	IM_VSDTRF_LOCATION_GR,
	IM_VSDTRF_LOCATION_RG,
	IM_VSDTRF_LOCATION_YY,
	// Location with discard mark:
	IM_VSDTRFd_LOCATION_GG,
	IM_VSDTRFd_LOCATION_GR,
	IM_VSDTRFd_LOCATION_RG,
	IM_VSDTRFd_LOCATION_YY,
	// Database:
	IM_VSDTRF_DATABASE_GG,
	IM_VSDTRF_DATABASE_GR,
	IM_VSDTRF_DATABASE_RG,
	IM_VSDTRF_DATABASE_YY,
	// Database with discard mark:
	IM_VSDTRFd_DATABASE_GG,
	IM_VSDTRFd_DATABASE_GR,
	IM_VSDTRFd_DATABASE_RG,
	IM_VSDTRFd_DATABASE_YY,
	// Database Distributed:
	IM_VSDTRF_DATABASE_DISTRIBUTED_GG,
	IM_VSDTRF_DATABASE_DISTRIBUTED_GR,
	IM_VSDTRF_DATABASE_DISTRIBUTED_RG,
	IM_VSDTRF_DATABASE_DISTRIBUTED_YY,
	// Database Distributed with discard mark:
	IM_VSDTRFd_DATABASE_DISTRIBUTED_GG,
	IM_VSDTRFd_DATABASE_DISTRIBUTED_GR,
	IM_VSDTRFd_DATABASE_DISTRIBUTED_RG,
	IM_VSDTRFd_DATABASE_DISTRIBUTED_YY,
	// Database Coordinator:
	IM_VSDTRF_DATABASE_COORDINATOR_GG,
	IM_VSDTRF_DATABASE_COORDINATOR_GR,
	IM_VSDTRF_DATABASE_COORDINATOR_RG,
	IM_VSDTRF_DATABASE_COORDINATOR_YY,
	// Database Coordinator with discard mark:
	IM_VSDTRFd_DATABASE_COORDINATOR_GG,
	IM_VSDTRFd_DATABASE_COORDINATOR_GR,
	IM_VSDTRFd_DATABASE_COORDINATOR_RG,
	IM_VSDTRFd_DATABASE_COORDINATOR_YY,
	// Table:
	IM_VSDTRF_TABLE_GG,
	IM_VSDTRF_TABLE_GR,
	IM_VSDTRF_TABLE_RG,
	IM_VSDTRF_TABLE_YY,
	// Table with discard mark:
	IM_VSDTRFd_TABLE_GG,
	IM_VSDTRFd_TABLE_GR,
	IM_VSDTRFd_TABLE_RG,
	IM_VSDTRFd_TABLE_YY,
	// Table star native:
	IM_VSDTRF_TABLE_NATIVE_GG,
	IM_VSDTRF_TABLE_NATIVE_GR,
	IM_VSDTRF_TABLE_NATIVE_RG,
	IM_VSDTRF_TABLE_NATIVE_YY,
	// Table star native with discard mark:
	IM_VSDTRFd_TABLE_NATIVE_GG,
	IM_VSDTRFd_TABLE_NATIVE_GR,
	IM_VSDTRFd_TABLE_NATIVE_RG,
	IM_VSDTRFd_TABLE_NATIVE_YY,
	// Table star Link:
	IM_VSDTRF_TABLE_LINK_GG,
	IM_VSDTRF_TABLE_LINK_GR,
	IM_VSDTRF_TABLE_LINK_RG,
	IM_VSDTRF_TABLE_LINK_YY,
	// Table star ling with discard mark:
	IM_VSDTRFd_TABLE_LINK_GG,
	IM_VSDTRFd_TABLE_LINK_GR,
	IM_VSDTRFd_TABLE_LINK_RG,
	IM_VSDTRFd_TABLE_LINK_YY,
	// View:
	IM_VSDTRF_VIEW_GG,
	IM_VSDTRF_VIEW_GR,
	IM_VSDTRF_VIEW_RG,
	IM_VSDTRF_VIEW_YY,
	// View with discard mark:
	IM_VSDTRFd_VIEW_GG,
	IM_VSDTRFd_VIEW_GR,
	IM_VSDTRFd_VIEW_RG,
	IM_VSDTRFd_VIEW_YY,
	// View star native:
	IM_VSDTRF_VIEW_NATIVE_GG,
	IM_VSDTRF_VIEW_NATIVE_GR,
	IM_VSDTRF_VIEW_NATIVE_RG,
	IM_VSDTRF_VIEW_NATIVE_YY,
	// View star native with discard mark:
	IM_VSDTRFd_VIEW_NATIVE_GG,
	IM_VSDTRFd_VIEW_NATIVE_GR,
	IM_VSDTRFd_VIEW_NATIVE_RG,
	IM_VSDTRFd_VIEW_NATIVE_YY,
	// View star register as link:
	IM_VSDTRF_VIEW_LINK_GG,
	IM_VSDTRF_VIEW_LINK_GR,
	IM_VSDTRF_VIEW_LINK_RG,
	IM_VSDTRF_VIEW_LINK_YY,
	// View star register as link with discard mark:
	IM_VSDTRFd_VIEW_LINK_GG,
	IM_VSDTRFd_VIEW_LINK_GR,
	IM_VSDTRFd_VIEW_LINK_RG,
	IM_VSDTRFd_VIEW_LINK_YY,
	// Procedure:
	IM_VSDTRF_PROCEDURE_GG,
	IM_VSDTRF_PROCEDURE_GR,
	IM_VSDTRF_PROCEDURE_RG,
	IM_VSDTRF_PROCEDURE_YY,
	// Procedure with discard mark:
	IM_VSDTRFd_PROCEDURE_GG,
	IM_VSDTRFd_PROCEDURE_GR,
	IM_VSDTRFd_PROCEDURE_RG,
	IM_VSDTRFd_PROCEDURE_YY,
	// Procedure register as link:
	IM_VSDTRF_PROCEDURE_LINK_GG,
	IM_VSDTRF_PROCEDURE_LINK_GR,
	IM_VSDTRF_PROCEDURE_LINK_RG,
	IM_VSDTRF_PROCEDURE_LINK_YY,
	// Procedure register as link with discard mark:
	IM_VSDTRFd_PROCEDURE_LINK_GG,
	IM_VSDTRFd_PROCEDURE_LINK_GR,
	IM_VSDTRFd_PROCEDURE_LINK_RG,
	IM_VSDTRFd_PROCEDURE_LINK_YY,
	// Sequence:
	IM_VSDTRF_SEQUENCE_GG,
	IM_VSDTRF_SEQUENCE_GR,
	IM_VSDTRF_SEQUENCE_RG,
	IM_VSDTRF_SEQUENCE_YY,
	// Sequence with discard mark:
	IM_VSDTRFd_SEQUENCE_GG,
	IM_VSDTRFd_SEQUENCE_GR,
	IM_VSDTRFd_SEQUENCE_RG,
	IM_VSDTRFd_SEQUENCE_YY,
	// Dbevent:
	IM_VSDTRF_DBEVENT_GG,
	IM_VSDTRF_DBEVENT_GR,
	IM_VSDTRF_DBEVENT_RG,
	IM_VSDTRF_DBEVENT_YY,
	// DBevent with discard mark:
	IM_VSDTRFd_DBEVENT_GG,
	IM_VSDTRFd_DBEVENT_GR,
	IM_VSDTRFd_DBEVENT_RG,
	IM_VSDTRFd_DBEVENT_YY,
	// Synonym:
	IM_VSDTRF_SYNONYM_GG,
	IM_VSDTRF_SYNONYM_GR,
	IM_VSDTRF_SYNONYM_RG,
	IM_VSDTRF_SYNONYM_YY,
	// Synonym with discard mark:
	IM_VSDTRFd_SYNONYM_GG,
	IM_VSDTRFd_SYNONYM_GR,
	IM_VSDTRFd_SYNONYM_RG,
	IM_VSDTRFd_SYNONYM_YY,
	// Index:
	IM_VSDTRF_INDEX_GG,
	IM_VSDTRF_INDEX_GR,
	IM_VSDTRF_INDEX_RG,
	IM_VSDTRF_INDEX_YY,
	// Index with discard mark:
	IM_VSDTRFd_INDEX_GG,
	IM_VSDTRFd_INDEX_GR,
	IM_VSDTRFd_INDEX_RG,
	IM_VSDTRFd_INDEX_YY,
	// Index Star Register As Link:
	IM_VSDTRF_INDEX_LINK_GG,
	IM_VSDTRF_INDEX_LINK_GR,
	IM_VSDTRF_INDEX_LINK_RG,
	IM_VSDTRF_INDEX_LINK_YY,
	// Index Star Register As Link with discard mark:
	IM_VSDTRFd_INDEX_LINK_GG,
	IM_VSDTRFd_INDEX_LINK_GR,
	IM_VSDTRFd_INDEX_LINK_RG,
	IM_VSDTRFd_INDEX_LINK_YY,
	// Rule:
	IM_VSDTRF_RULE_GG,
	IM_VSDTRF_RULE_GR,
	IM_VSDTRF_RULE_RG,
	IM_VSDTRF_RULE_YY,
	// Rule with discard mark:
	IM_VSDTRFd_RULE_GG,
	IM_VSDTRFd_RULE_GR,
	IM_VSDTRFd_RULE_RG,
	IM_VSDTRFd_RULE_YY,
	// Integrity:
	IM_VSDTRF_INTEGRITY_GG,
	IM_VSDTRF_INTEGRITY_GR,
	IM_VSDTRF_INTEGRITY_RG,
	IM_VSDTRF_INTEGRITY_YY,
	// Integrity with discard mark:
	IM_VSDTRFd_INTEGRITY_GG,
	IM_VSDTRFd_INTEGRITY_GR,
	IM_VSDTRFd_INTEGRITY_RG,
	IM_VSDTRFd_INTEGRITY_YY,

	// Security Alarm of Installation:
	IM_VSDTRF_AxINSTALLATION_GG,
	IM_VSDTRF_AxINSTALLATION_GR,
	IM_VSDTRF_AxINSTALLATION_RG,
	IM_VSDTRF_AxINSTALLATION_YY,
	// Security Alarm of installation with discard mark:
	IM_VSDTRFd_AxINSTALLATION_GG,
	IM_VSDTRFd_AxINSTALLATION_GR,
	IM_VSDTRFd_AxINSTALLATION_RG,
	IM_VSDTRFd_AxINSTALLATION_YY,
	// Security Alarm of database:
	IM_VSDTRF_AxDATABASE_GG,
	IM_VSDTRF_AxDATABASE_GR,
	IM_VSDTRF_AxDATABASE_RG,
	IM_VSDTRF_AxDATABASE_YY,
	// Security Alarm of database with discard mark:
	IM_VSDTRFd_AxDATABASE_GG,
	IM_VSDTRFd_AxDATABASE_GR,
	IM_VSDTRFd_AxDATABASE_RG,
	IM_VSDTRFd_AxDATABASE_YY,
	// Security Alarm of Table:
	IM_VSDTRF_AxTABLE_GG,
	IM_VSDTRF_AxTABLE_GR,
	IM_VSDTRF_AxTABLE_RG,
	IM_VSDTRF_AxTABLE_YY,
	// Security Alarm of Table with discard mark:
	IM_VSDTRFd_AxTABLE_GG,
	IM_VSDTRFd_AxTABLE_GR,
	IM_VSDTRFd_AxTABLE_RG,
	IM_VSDTRFd_AxTABLE_YY,

	// Grantee for installation:
	IM_VSDTRF_GxINSTALLATION_GG,
	IM_VSDTRF_GxINSTALLATION_GR,
	IM_VSDTRF_GxINSTALLATION_RG,
	IM_VSDTRF_GxINSTALLATION_YY,
	// Grantee for installation with discard mark:
	IM_VSDTRFd_GxINSTALLATION_GG,
	IM_VSDTRFd_GxINSTALLATION_GR,
	IM_VSDTRFd_GxINSTALLATION_RG,
	IM_VSDTRFd_GxINSTALLATION_YY,
	// Grantee for database:
	IM_VSDTRF_GxDATABASE_GG,
	IM_VSDTRF_GxDATABASE_GR,
	IM_VSDTRF_GxDATABASE_RG,
	IM_VSDTRF_GxDATABASE_YY,
	// Grantee for database with discard mark:
	IM_VSDTRFd_GxDATABASE_GG,
	IM_VSDTRFd_GxDATABASE_GR,
	IM_VSDTRFd_GxDATABASE_RG,
	IM_VSDTRFd_GxDATABASE_YY,
	// Grantee for table:
	IM_VSDTRF_GxTABLE_GG,
	IM_VSDTRF_GxTABLE_GR,
	IM_VSDTRF_GxTABLE_RG,
	IM_VSDTRF_GxTABLE_YY,
	// Grantee for table with discard mark:
	IM_VSDTRFd_GxTABLE_GG,
	IM_VSDTRFd_GxTABLE_GR,
	IM_VSDTRFd_GxTABLE_RG,
	IM_VSDTRFd_GxTABLE_YY,
	// Grantee for view:
	IM_VSDTRF_GxVIEW_GG,
	IM_VSDTRF_GxVIEW_GR,
	IM_VSDTRF_GxVIEW_RG,
	IM_VSDTRF_GxVIEW_YY,
	// Grantee for View with discard mark:
	IM_VSDTRFd_GxVIEW_GG,
	IM_VSDTRFd_GxVIEW_GR,
	IM_VSDTRFd_GxVIEW_RG,
	IM_VSDTRFd_GxVIEW_YY,
	// Grantee for procedure:
	IM_VSDTRF_GxPROCEDURE_GG,
	IM_VSDTRF_GxPROCEDURE_GR,
	IM_VSDTRF_GxPROCEDURE_RG,
	IM_VSDTRF_GxPROCEDURE_YY,
	// Grantee for procedure with discard mark:
	IM_VSDTRFd_GxPROCEDURE_GG,
	IM_VSDTRFd_GxPROCEDURE_GR,
	IM_VSDTRFd_GxPROCEDURE_RG,
	IM_VSDTRFd_GxPROCEDURE_YY,
	// Grantee for Sequence:
	IM_VSDTRF_GxSEQUENCE_GG,
	IM_VSDTRF_GxSEQUENCE_GR,
	IM_VSDTRF_GxSEQUENCE_RG,
	IM_VSDTRF_GxSEQUENCE_YY,
	// Grantee for Sequence with discard mark:
	IM_VSDTRFd_GxSEQUENCE_GG,
	IM_VSDTRFd_GxSEQUENCE_GR,
	IM_VSDTRFd_GxSEQUENCE_RG,
	IM_VSDTRFd_GxSEQUENCE_YY,
	// Grantee for dbevent:
	IM_VSDTRF_GxDBEVENT_GG,
	IM_VSDTRF_GxDBEVENT_GR,
	IM_VSDTRF_GxDBEVENT_RG,
	IM_VSDTRF_GxDBEVENT_YY,
	// Grantee for dbevent with discard mark:
	IM_VSDTRFd_GxDBEVENT_GG,
	IM_VSDTRFd_GxDBEVENT_GR,
	IM_VSDTRFd_GxDBEVENT_RG,
	IM_VSDTRFd_GxDBEVENT_YY
};

class CaVsdaLoadSchema;
class CaVsdRefreshTreeInfo : public CaRefreshTreeInfo
{
public:
	typedef enum {ACTION_QUERY=0, ACTION_REFRESH} RefreshAction;

	CaVsdRefreshTreeInfo(RefreshAction nAction = ACTION_QUERY):CaRefreshTreeInfo()
	{
		Initialize();
		m_bLoadSchema1 = FALSE;
		m_bLoadSchema2 = FALSE;
		m_bIgnorOwner  = TRUE;
		m_bInstallation = FALSE;
		m_hWndAnimateDlg = NULL;
		m_bInterrupted = FALSE;
	}
	~CaVsdRefreshTreeInfo(){}
	void SetInstallation(BOOL bSet){m_bInstallation = bSet;}
	void SetNode (LPCTSTR lpszNode1, LPCTSTR lpszNode2)
	{
		m_strNode1 = lpszNode1;
		m_strNode2 = lpszNode2;
	}
	void SetDatabase (LPCTSTR lpszDatabase1, LPCTSTR lpszDatabase2)
	{
		m_strDatabase1 = lpszDatabase1;
		m_strDatabase2 = lpszDatabase2;
	}
	void SetSchema (LPCTSTR lpszSchema1, LPCTSTR lpszSchema2)
	{
		m_strSchema1 = lpszSchema1;
		m_strSchema2 = lpszSchema2;
	}
	void SetLoadSchema(LPCTSTR lpszFile1, LPCTSTR lpszFile2)
	{
		m_LoadedSchema1.SetFile(lpszFile1);
		m_LoadedSchema2.SetFile(lpszFile2);
		m_bLoadSchema1 = (m_LoadedSchema1.GetFile().IsEmpty())? FALSE: TRUE;
		m_bLoadSchema2 = (m_LoadedSchema2.GetFile().IsEmpty())? FALSE: TRUE;
	}
	void SetIgnoreOwner(BOOL bSet){m_bIgnorOwner = bSet;}
	void SetHwndAnimate(HWND hWndTaskDlg){m_hWndAnimateDlg = hWndTaskDlg;}
	HWND GetHwndAnimate(){return m_hWndAnimateDlg;}

	CString GetNode1();
	CString GetNode2();
	CString GetDatabase1();
	CString GetDatabase2();
	CString GetSchema1();
	CString GetSchema2();
	UINT GetFlag(){return m_uiFlag;}
	void SetFlag(UINT uiFlag){ m_uiFlag = uiFlag;}

	BOOL IsLoadSchema1(){return m_bLoadSchema1;}
	BOOL IsLoadSchema2(){return m_bLoadSchema2;}
	BOOL IsIgnoreOwner(){return m_bIgnorOwner;}
	BOOL IsInstallation(){return m_bInstallation;}

	CaVsdaLoadSchema* GetLoadedSchema1(){return &m_LoadedSchema1;}
	CaVsdaLoadSchema* GetLoadedSchema2(){return &m_LoadedSchema2;}
	void Close()
	{
		m_LoadedSchema1.Close(); 
		m_LoadedSchema2.Close();
	}
	void Initialize()
	{
		m_strNode1 = _T("");
		m_strNode2 = _T("");
		m_strDatabase1 = _T("");
		m_strDatabase2 = _T("");
		m_strSchema1 = _T("");
		m_strSchema2 = _T("");
		m_bLoadSchema1 = FALSE;
		m_bLoadSchema2 = FALSE;
		m_bIgnorOwner = FALSE;
		m_bInstallation = FALSE;
		m_hWndAnimateDlg = NULL;
		m_uiFlag = 0;
		m_bInterrupted = FALSE;
		Close();
	}
	void ShowAnimateTextInfo(LPCTSTR lpszText); 
	void Interrupt(BOOL bSet){m_bInterrupted = bSet;}
	BOOL IsInterrupted(){return m_bInterrupted;}

protected:
	CString m_strNode1;
	CString m_strNode2;
	CString m_strDatabase1;
	CString m_strDatabase2;
	CString m_strSchema1;
	CString m_strSchema2;

	BOOL m_bLoadSchema1;
	BOOL m_bLoadSchema2;
	BOOL m_bIgnorOwner;
	BOOL m_bInstallation;

	CaVsdaLoadSchema m_LoadedSchema1;
	CaVsdaLoadSchema m_LoadedSchema2;
	HWND m_hWndAnimateDlg;
	UINT m_uiFlag;
	BOOL m_bInterrupted;
};

class CaVsdDisplayInfo : public CaDisplayInfo
{
public:
	CaVsdDisplayInfo():CaDisplayInfo(){m_pRefreshTreeInfo = new CaVsdRefreshTreeInfo(CaVsdRefreshTreeInfo::ACTION_QUERY);}
	virtual ~CaVsdDisplayInfo(){if (m_pRefreshTreeInfo) delete m_pRefreshTreeInfo;}
	CArray <UINT, UINT>& GetUnDisplayItems(){return m_arrayUnDisplayItem;}

	CString GetCompareTitle1() {return m_strCompareTitle1;}
	CString GetCompareTitle2() {return m_strCompareTitle2;}
	void SetCompareTitles(LPCTSTR lpszT1, LPCTSTR lpszT2){m_strCompareTitle1=lpszT1; m_strCompareTitle2=lpszT2;}
	CaVsdRefreshTreeInfo* GetRefreshTreeInfo(){return m_pRefreshTreeInfo;}
protected:
	//
	// The elements of m_arrayUnDisplayItem are O_FOLDER_XXX, O_XXX. (EX: O_FOLDER_USER, O_GROUP, ...)
	CArray <UINT, UINT> m_arrayUnDisplayItem;

	CString m_strCompareTitle1;
	CString m_strCompareTitle2;
	CaVsdRefreshTreeInfo* m_pRefreshTreeInfo;
};

class CaVsdDifference : public CObject
{
public:
	CaVsdDifference(){m_bDetail = FALSE;}
	virtual ~CaVsdDifference(){}

	void SetSpecialDetail(BOOL bSet){m_bDetail = bSet;}
	void SetType (LPCTSTR lpszType){m_strType = lpszType;}
	void SetType ( UINT nID );
	void SetInfoSchema1 (LPCTSTR lpszInfo1){m_strSchema1 = lpszInfo1;}
	void SetInfoSchema2 (LPCTSTR lpszInfo2){m_strSchema2 = lpszInfo2;}
	void SetInfoSchema1 (int nSet);
	void SetInfoSchema2 (int nSet);
	void SetInfoSchema1 (TCHAR cSet);
	void SetInfoSchema2 (TCHAR cSet);

	BOOL    GetSpecialDetail(){return m_bDetail;}
	CString GetType(){return m_strType;}
	CString GetInfoSchema1(){return m_strSchema1;}
	CString GetInfoSchema2(){return m_strSchema2;}
protected:
	BOOL    m_bDetail;     // TRUE -> Enable using property frame for special detail
	CString m_strType;     // Missing Object, Structure, ...
	CString m_strSchema1;  // Available, N/A, ...
	CString m_strSchema2;  // Available, N/A, ...
};

inline void CaVsdDifference::SetType (UINT nID)
{
	m_strType.LoadString(nID);
}

inline void CaVsdDifference::SetInfoSchema1 (int nSet)
{
	m_strSchema1.Format(_T("%d"), nSet);
}
inline void CaVsdDifference::SetInfoSchema2 (int nSet)
{
	m_strSchema2.Format(_T("%d"), nSet);
}

inline void CaVsdDifference::SetInfoSchema1 (TCHAR cSet)
{
	m_strSchema1 = cSet;
}
inline void CaVsdDifference::SetInfoSchema2 (TCHAR cSet)
{
	m_strSchema2 = cSet;
}



class CtrfItemData;
class CaVsdItemData
{
public:
	CaVsdItemData(CtrfItemData* pBackParent = NULL);
	~CaVsdItemData();

	void VsdFolder(CtrfItemData* pBackParent)
	{
		m_pBackObject = pBackParent;
		m_bVsdFolder = m_pBackObject? TRUE: FALSE;
	};
	void VsdItem(CtrfItemData* pBackParent)
	{
		m_pBackObject = pBackParent;
		m_bVsdFolder = FALSE;
	};
	TCHAR& GetDifference(){return m_tchszDifference;}
	void SetMultiple(BOOL bSet = TRUE){m_bMultiple = bSet;}
	BOOL IsMultiple(){return m_bMultiple;}
	CString GetOwner2(){return m_strOwner2;}
	void SetOwner2(LPCTSTR lpszOwner){m_strOwner2 = lpszOwner;}
	CTypedPtrList< CObList, CaVsdDifference* >& GetListDifference(){return m_listDifference;}
	virtual CString GetName(){return _T("");}
	virtual CString GetOwner(){return _T("");}
	virtual TCHAR HasDifferences (BOOL bIncludeSubFolder = TRUE);

	void SetDiscard(BOOL bSet){m_bDiscarded = bSet;}
	BOOL IsDiscarded(){return m_bDiscarded;}
protected:
	//
	// m_tchszDifference can be:
	//    '+': exist in schema1 and not exist in schema2
	//    '-': exist in schema2 and not exist in schema1
	//    '*': object's name is identical (need to compare the detail and sub-branch)
	//    '#': object's name and object's owner are identical (need to compare the detail and sub-branch)
	TCHAR m_tchszDifference; 
	BOOL  m_bMultiple;
	CString m_strOwner2;
	BOOL  m_bVsdFolder;
	BOOL  m_bDiscarded;
	CtrfItemData* m_pBackObject;

	CTypedPtrList< CObList, CaVsdDifference* > m_listDifference;
};

//
// Object: Security Alarm 
// ************************************************************************************************
class CaVsdAlarm: public CtrfAlarm, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdAlarm)
public:
	CaVsdAlarm():CtrfAlarm(){VsdItem(this);}
	CaVsdAlarm(CaAlarm* pObj): CtrfAlarm(pObj){VsdItem(this);}
	virtual ~CaVsdAlarm(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){return GetObject().GetOwner();}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
	virtual CString GetDisplayedItem(int nMode = 0);
#endif
};

//
// Object: Folder of Security Alarm  
// ************************************************************************************************
class CaVsdFolderAlarm: public CtrfFolderAlarm, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderAlarm)
	CaVsdFolderAlarm():CtrfFolderAlarm(){VsdFolder(this);}
public:
	CaVsdFolderAlarm(int nObjectParentType):CtrfFolderAlarm(nObjectParentType){VsdFolder(this);}
	virtual ~CaVsdFolderAlarm(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}

	CaVsdItemData* GetVsdParentItem(){return (CaVsdItemData*)m_pBackParent->GetUserData();}
};

//
// Object: Grantee 
// ************************************************************************************************
class CaVsdGrantee: public CtrfGrantee, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdGrantee)
public:
	CaVsdGrantee():CtrfGrantee(){VsdItem(this);}
	CaVsdGrantee(CaGrantee* pObj): CtrfGrantee(pObj){VsdItem(this);}
	virtual ~CaVsdGrantee(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){ASSERT(FALSE); return _T("");}
	virtual CString GetDisplayedItem(int nMode = 0);
};

//
// Object: Folder of Grantee 
// ************************************************************************************************
class CaVsdFolderGrantee: public CtrfFolderGrantee, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderGrantee)
	CaVsdFolderGrantee():CtrfFolderGrantee(){VsdFolder(this);}
public:
	CaVsdFolderGrantee(int nObjectParentType):CtrfFolderGrantee(nObjectParentType){VsdFolder(this);}
	virtual ~CaVsdFolderGrantee(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}

	CaVsdItemData* GetVsdParentItem(){return (CaVsdItemData*)m_pBackParent->GetUserData();}
};


//
// Object: Table 
// ************************************************************************************************
class CaVsdTable : public CtrfTable, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdTable)
public:
	CaVsdTable(): CtrfTable(), m_bInitialized(FALSE){VsdItem(this);}
	CaVsdTable(CaTable* pObj): CtrfTable(pObj), m_bInitialized(FALSE){VsdItem(this);}
	void Initialize();
	virtual ~CaVsdTable(){};
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){}
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){return GetObject().GetOwner();}
	virtual CString GetDisplayedItem(int nMode = 0);
	void SetFlag(UINT iFlag){m_uiFlag = iFlag;}
	UINT GetFlag(){return m_uiFlag;}

protected:
	BOOL m_bInitialized;
	UINT m_uiFlag;

};



//
// Object: Folder of Table 
// ************************************************************************************************
class CaVsdFolderTable : public CtrfFolderTable, public CaVsdItemData
{
	DECLARE_SERIAL (CaVsdFolderTable)
public:
	CaVsdFolderTable():CtrfFolderTable(){VsdFolder(this);}
	virtual ~CaVsdFolderTable(){}
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};



//
// Object: View 
// ************************************************************************************************
class CaVsdView: public CtrfView, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdView)
public:
	CaVsdView(): CtrfView(), m_bInitView(FALSE){VsdItem(this);}
	CaVsdView(CaView* pObj):CtrfView(pObj), m_bInitView(FALSE){VsdItem(this);}

	void Initialize();
	virtual ~CaVsdView(){};
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){return GetObject().GetOwner();}
	virtual CString GetDisplayedItem(int nMode = 0);
	void SetFlag(UINT iFlag){m_uiFlag = iFlag;}
	UINT GetFlag(){return m_uiFlag;}
protected:
	BOOL m_bInitView;
	UINT m_uiFlag;
};

//
// Object: Folder of View 
// ************************************************************************************************
class CaVsdFolderView: public CtrfFolderView, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderView)
public:
	CaVsdFolderView():CtrfFolderView(){VsdFolder(this);}
	virtual ~CaVsdFolderView(){}

	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};


//
// Object: Procedure 
// ************************************************************************************************
class CaVsdProcedure: public CtrfProcedure, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdProcedure)
public:
	CaVsdProcedure():CtrfProcedure(), m_bInitProcedure(FALSE){VsdItem(this);}
	CaVsdProcedure(CaProcedure* pObj): CtrfProcedure(pObj), m_bInitProcedure(FALSE){VsdItem(this);}
	void Initialize();
	virtual ~CaVsdProcedure(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){return GetObject().GetOwner();}
	void SetFlag(UINT iFlag){m_uiFlag = iFlag;}
	UINT GetFlag(){return m_uiFlag;}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
	virtual CString GetDisplayedItem(int nMode = 0);
#endif
protected:
	BOOL m_bInitProcedure;
	UINT m_uiFlag;
};


//
// Object: Folder of Procedure 
// ************************************************************************************************
class CaVsdFolderProcedure: public CtrfFolderProcedure, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderProcedure)
public:
	CaVsdFolderProcedure():CtrfFolderProcedure(){VsdFolder(this);}
	virtual ~CaVsdFolderProcedure(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};

//
// Object: Sequence 
// ************************************************************************************************
class CaVsdSequence: public CtrfSequence, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdSequence)
public:
	CaVsdSequence():CtrfSequence(), m_bInitSequence(FALSE){VsdItem(this);}
	CaVsdSequence(CaSequence* pObj): CtrfSequence(pObj), m_bInitSequence(FALSE){VsdItem(this);}
	void Initialize();
	virtual ~CaVsdSequence(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){return GetObject().GetOwner();}
	void SetFlag(UINT iFlag){m_uiFlag = iFlag;}
	UINT GetFlag(){return m_uiFlag;}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
	virtual CString GetDisplayedItem(int nMode = 0);
#endif
protected:
	BOOL m_bInitSequence;
	UINT m_uiFlag;
};


//
// Object: Folder of Sequence 
// ************************************************************************************************
class CaVsdFolderSequence: public CtrfFolderSequence, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderSequence)
public:
	CaVsdFolderSequence():CtrfFolderSequence(){VsdFolder(this);}
	virtual ~CaVsdFolderSequence(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};

//
// Object: DBEvent:
// ************************************************************************************************
class CaVsdDBEvent: public CtrfDBEvent, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdDBEvent)
public:
	CaVsdDBEvent():CtrfDBEvent(), m_bInitDBEvent(FALSE){VsdItem(this);}
	CaVsdDBEvent(CaDBEvent* pObj):CtrfDBEvent(pObj), m_bInitDBEvent(FALSE){VsdItem(this);}
	void Initialize();
	virtual ~CaVsdDBEvent(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){return GetObject().GetOwner();}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
	virtual CString GetDisplayedItem(int nMode = 0);
#endif
protected:
	BOOL m_bInitDBEvent;
};


//
// Folder of DBEvent:
// ************************************************************************************************
class CaVsdFolderDBEvent: public CtrfFolderDBEvent, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderDBEvent)
public:
	CaVsdFolderDBEvent():CtrfFolderDBEvent(){VsdFolder(this);}
	virtual ~CaVsdFolderDBEvent(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};


//
// Object: Synonym
// ************************************************************************************************
class CaVsdSynonym: public CtrfSynonym, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdSynonym)
public:
	CaVsdSynonym():CtrfSynonym(){VsdItem(this);}
	CaVsdSynonym(CaSynonym* pObj):CtrfSynonym(pObj){VsdItem(this);}
	virtual ~CaVsdSynonym(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){return GetObject().GetOwner();}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
	virtual CString GetDisplayedItem(int nMode = 0);
#endif
};


//
// Folder of Synonym
// ************************************************************************************************
class CaVsdFolderSynonym: public CtrfFolderSynonym, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderSynonym)
public:
	CaVsdFolderSynonym():CtrfFolderSynonym(){VsdFolder(this);}
	virtual ~CaVsdFolderSynonym(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};


//
// Object: Index 
// ************************************************************************************************
class CaVsdIndex: public CtrfIndex, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdIndex)
public:
	CaVsdIndex(): CtrfIndex(){VsdItem(this);}
	CaVsdIndex(CaIndex* pObj): CtrfIndex(pObj){VsdItem(this);}
	virtual ~CaVsdIndex(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName();
	virtual CString GetOwner();
	virtual CString GetDisplayedItem(int nMode = 0);
};


//
// Object: Folder of Index 
// ************************************************************************************************
class CaVsdFolderIndex: public CtrfFolderIndex, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderIndex)
public:
	CaVsdFolderIndex():CtrfFolderIndex(){VsdFolder(this);}
	virtual ~CaVsdFolderIndex(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};


//
// Object: RULE
// ************************************************************************************************
class CaVsdRule: public CtrfRule, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdRule)
public:
	CaVsdRule():CtrfRule(){VsdItem(this);}
	CaVsdRule(CaRule* pObj): CtrfRule(pObj){VsdItem(this);}
	virtual ~CaVsdRule(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName();
	virtual CString GetOwner();
	virtual CString GetDisplayedItem(int nMode = 0);
};



//
// Folder of Rule:
// ************************************************************************************************
class CaVsdFolderRule: public CtrfFolderRule, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderRule)
public:
	CaVsdFolderRule():CtrfFolderRule(){VsdFolder(this);}
	virtual ~CaVsdFolderRule(){}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};


//
// Object: Integrity
// ************************************************************************************************
class CaVsdIntegrity: public CtrfIntegrity, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdIntegrity)
public:
	CaVsdIntegrity(): CtrfIntegrity(){VsdItem(this);}
	CaVsdIntegrity(CaIntegrity* pObj): CtrfIntegrity(pObj){VsdItem(this);}
	virtual ~CaVsdIntegrity(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName();
	virtual CString GetOwner();
	virtual CString GetDisplayedItem(int nMode = 0);
};


//
// Folder of Integrity:
// ************************************************************************************************
class CaVsdFolderIntegrity: public CtrfFolderIntegrity, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderIntegrity)
public:
	CaVsdFolderIntegrity():CtrfFolderIntegrity(){VsdFolder(this);}
	virtual ~CaVsdFolderIntegrity(){}

	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};

class CaVsdDatabase: public CtrfDatabase, public CaVsdItemData
{
	DECLARE_SERIAL (CaVsdDatabase)
public:
	CaVsdDatabase():CtrfDatabase(), m_bInitialized(FALSE){VsdItem(this);}
	CaVsdDatabase(CaDatabase* pObj):CtrfDatabase(pObj), m_bInitialized(FALSE){VsdItem(this);}
	virtual ~CaVsdDatabase();
	void Initialize();
	void Cleanup();
	BOOL Populate (CaVsdRefreshTreeInfo* pPopulateData);

	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){return GetObject().GetOwner();}
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetDisplayedItem(int nMode = 0);
protected:
	BOOL m_bInitialized;
	CString m_strDisplayItem;
};

//
// Folder of Database:
// ************************************************************************************************
class CaVsdFolderDatabase: public CtrfFolderDatabase, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderDatabase)
public:
	CaVsdFolderDatabase():CtrfFolderDatabase(){VsdFolder(this);}
	virtual ~CaVsdFolderDatabase(){}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};


//
// Object: User 
// ************************************************************************************************
class CaVsdUser: public CtrfUser, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdUser)
public:
	CaVsdUser():CtrfUser(){VsdItem(this);}
	CaVsdUser(CaUser* pObj): CtrfUser(pObj){VsdItem(this);}
	virtual ~CaVsdUser(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){ASSERT(FALSE); return GetObject().GetOwner();}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
	virtual CString GetDisplayedItem(int nMode = 0);
#endif
};

//
// Object: Folder of User 
// ************************************************************************************************
class CaVsdFolderUser: public CtrfFolderUser, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderUser)
public:
	CaVsdFolderUser():CtrfFolderUser(){VsdFolder(this);}
	virtual ~CaVsdFolderUser(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};


//
// Object: Group 
// ************************************************************************************************
class CaVsdGroup: public CtrfGroup, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdGroup)
public:
	CaVsdGroup():CtrfGroup(){VsdItem(this);}
	CaVsdGroup(CaGroup* pObj): CtrfGroup(pObj){VsdItem(this);}
	virtual ~CaVsdGroup(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){ASSERT(FALSE); return GetObject().GetOwner();}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
	virtual CString GetDisplayedItem(int nMode = 0);
#endif
};

//
// Object: Folder of Group 
// ************************************************************************************************
class CaVsdFolderGroup: public CtrfFolderGroup, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderGroup)
public:
	CaVsdFolderGroup():CtrfFolderGroup(){VsdFolder(this);}
	virtual ~CaVsdFolderGroup(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};


//
// Object: Role 
// ************************************************************************************************
class CaVsdRole: public CtrfRole, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdRole)
public:
	CaVsdRole():CtrfRole(){VsdItem(this);}
	CaVsdRole(CaRole* pObj): CtrfRole(pObj){VsdItem(this);}
	virtual ~CaVsdRole(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){ASSERT(FALSE); return GetObject().GetOwner();}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
	virtual CString GetDisplayedItem(int nMode = 0);
#endif
};

//
// Object: Folder of Role 
// ************************************************************************************************
class CaVsdFolderRole: public CtrfFolderRole, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderRole)
public:
	CaVsdFolderRole():CtrfFolderRole(){VsdFolder(this);}
	virtual ~CaVsdFolderRole(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};


//
// Object: Profile 
// ************************************************************************************************
class CaVsdProfile: public CtrfProfile, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdProfile)
public:
	CaVsdProfile():CtrfProfile(){VsdItem(this);}
	CaVsdProfile(CaProfile* pObj): CtrfProfile(pObj){VsdItem(this);}
	virtual ~CaVsdProfile(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){ASSERT(FALSE); return GetObject().GetOwner();}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
	virtual CString GetDisplayedItem(int nMode = 0);
#endif
};

//
// Object: Folder of Profile 
// ************************************************************************************************
class CaVsdFolderProfile: public CtrfFolderProfile, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderProfile)
public:
	CaVsdFolderProfile():CtrfFolderProfile(){VsdFolder(this);}
	virtual ~CaVsdFolderProfile(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};


//
// Object: Location 
// ************************************************************************************************
class CaVsdLocation: public CtrfLocation, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdLocation)
public:
	CaVsdLocation():CtrfLocation(){VsdItem(this);}
	CaVsdLocation(CaLocation* pObj): CtrfLocation(pObj){VsdItem(this);}
	virtual ~CaVsdLocation(){}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetName(){return GetObject().GetName();}
	virtual CString GetOwner(){ASSERT(FALSE); return GetObject().GetOwner();}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
	virtual CString GetDisplayedItem(int nMode = 0);
#endif
};

//
// Object: Folder of Location 
// ************************************************************************************************
class CaVsdFolderLocation: public CtrfFolderLocation, public CaVsdItemData
{
	DECLARE_SERIAL(CaVsdFolderLocation)
public:
	CaVsdFolderLocation():CtrfFolderLocation(){VsdFolder(this);}
	virtual ~CaVsdFolderLocation(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand){CtrfItemData::Expand (pTree, hExpand);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
};


//
// Object: ROOT (Installation)
// ************************************************************************************************
class CaVsdRoot: public CtrfItemData, public CaVsdItemData
{
	DECLARE_SERIAL (CaVsdRoot)
public:
	CaVsdRoot():CtrfItemData(O_FOLDER_INSTALLATION)
	{
		VsdFolder(this);
		GetDBObject()->SetObjectID(OBT_INSTALLATION);
		m_queryInfo.SetLockMode(LM_NOLOCK);
	}
	virtual ~CaVsdRoot();
	void Initialize(BOOL bInstallation);
	void Cleanup();
	BOOL Populate (CaVsdRefreshTreeInfo* pPopulateData);

	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void SetSessionManager(CmtSessionManager* pSessionManager){m_pSessionManager=pSessionManager;}
	virtual CmtSessionManager* GetSessionManager(){return m_pSessionManager;}
	virtual CaDisplayInfo* GetDisplayInfo(){return &m_displayInfo;}
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL)
	{
		//
		// The systems object flags must not be re-initialized
		BOOL bSystemObject = m_queryInfo.GetIncludeSystemObjects();
		if (pData)
		{
			m_queryInfo.Initialize();
			m_queryInfo = *pData;
			m_queryInfo.SetLockMode(LM_NOLOCK);
		}

		//
		// The systems object flags must not be re-initialized
		m_queryInfo.SetIncludeSystemObjects(bSystemObject);
		return &m_queryInfo;
	}
	virtual CString GetName(){return _T("Installation");}
	virtual void* GetUserData(){return (CaVsdItemData*)this;}
	virtual CString GetDisplayedItem(int nMode = 0){return m_strDisplayItem;}
	CaVsdDatabase* SetSchemaDatabase(LPCTSTR lpszNatabase);
	CaVsdDatabase* FindFirstDatabase();

protected:
	CaVsdDisplayInfo   m_displayInfo;         // Information for displaying tree folders
	CaLLQueryInfo      m_queryInfo;           // Query Information.
	CmtSessionManager* m_pSessionManager;     // If the Query Object is not done through ICAS COM Server.
	CString m_strDisplayItem;
};


#endif // VSD_TREE_HEADER
