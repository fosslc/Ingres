/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dmlurp.cpp, Implementation File 
**    Project  : Ingres II / VDBA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data manipulation (User, Role, Profile)
**
** History:
**
** 26-Oct-1999 (uk$so01)
**    Created, Rewrite the dialog code, in C++/MFC.
** 31-Dec-1999 (uk$so01)
**    Add the Get Session block of code in the constructor:
**    CaProfile::CaProfile(int nNodeHandle, BOOL bCreate, void* lpProfileParam)
** 24-Feb-2000 (uk$so01)
**    Bug #100557: Remove the (default profile) from the create user dialog box
** 14-Sep-2000 (uk$so01)
**    Bug #102609: Pop help ID when destroying the dialog
** 15-Jun-2001(schph01)
**    SIR 104991 add new method GrantRevoke4Rmcmd() to grant or revoke the
**      user for rmcmd views,dbevents,procedures.
** 31-Jul-2001 (schph01)
**  (bug 105349) when opening a session in imadb for managing the grants on
**  rmcmd, use SESSION_TYPE_INDEPENDENT instead of SESSION_TYPE_CACHENOREADLOCK
**  because SESSION_TYPE_CACHENOREADLOCK, in the case of imadb, executes the
**  ima procedures (set_server_domain and ima_set_vnode_domain) which are not needed for 
**  managing the grants on rmcmd, and result in grant errors if the connected
**  user has insufficient rights on imadb
**  07-Oct-2002 (schph01)
**  bug 108883 fill the m_bOwnerRmcmdObjects member in CaUser class with
**  the value of iOwnerRmcmdObjects of USERPARAMS structure
**  22-Oct-2003 (schph01)
**  (SIR 111153) change sql statement for granting rmcmd objects.
**  07-Apr-2005 (srisu02)
**  Fill the bRemovePwd member in USERPARAMS structure with
**  the value of m_bRemovePwd of CaUser structure
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "urpectrl.h"
#include "dmlurp.h"
#include "xdlgprof.h"
#include "xdlgrole.h"
#include "xdlguser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#if !defined (BUFSIZE)
#define BUFSIZE     256
#endif

extern "C"
{
#include "msghandl.h"
#include "dlgres.h"
#include "dbaset.h"
#include "getdata.h"
#include "domdata.h"
#include "dbaginfo.h"

BOOL SQLGetDbmsSecurityB1Info();
}
#define ACCESS_LOWLEVELCALL

static BOOL Copy2ProfileParam  (CaProfile* pProfileObject, PROFILEPARAMS* pProfileParam);
static BOOL Copy2ProfileObject (PROFILEPARAMS* pProfileParam, CaProfile* pProfileObject);

static BOOL Copy2RoleParam  (CaRole* pRoleObject, ROLEPARAMS* pRoleParam);
static BOOL Copy2RoleObject (ROLEPARAMS* pRoleParam, CaRole* pRoleObject);

static BOOL Copy2UserParam  (CaUser* pUserObject, USERPARAMS* pUserParam);
static BOOL Copy2UserObject (USERPARAMS* pUserParam, CaUser* pUserObject);

BOOL INGRESII_IsDefaultProfile(LPCTSTR lpszTring)
{
	if (lstrcmpi (lpszTring, (LPCTSTR)lpdefprofiledispstring()) == 0)
		return TRUE;
	else
		return FALSE;
}

// ----------------------------------------------------------
// CREATE / ALTER PROFILE
// ----------------------------------------------------------
void CaProfile::InitPrivilege()
{
	CaURPPrivilegesItemData* pItem;

	pItem = new CaURPPrivilegesItemData(_T("Create Database"),   FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Trace"),             FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Security"),          FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Operator"),          FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Maintain Location"), FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Auditor"),           FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Maintain Audit"),    FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Maintain Users"),    FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);

	BOOL bSecurityB1 = SQLGetDbmsSecurityB1Info();
	if (!bSecurityB1)
		return;
	//
	// The MAC privileges:
	pItem = new CaURPPrivilegesItemData(_T("Write Down"),        FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Write Fixed"),       FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Write Up"),          FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Insert Down"),       FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Insert Up"),         FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
	pItem = new CaURPPrivilegesItemData(_T("Session Security Label"),    FALSE, FALSE);
	m_listPrivileges.AddTail (pItem);
}


CaProfile::CaProfile(int nNodeHandle, BOOL bCreate, void* lpProfileParam)
{
	m_nNodeHandle = nNodeHandle;
	m_bCreate = bCreate;
	m_pParam = lpProfileParam;

	m_Privilege = new BOOL[USERPRIVILEGES];
	m_bDefaultPrivilege = new BOOL[USERPRIVILEGES];
	m_bSecAudit = new BOOL[USERSECAUDITS];

	//
	// Get Session: InitPrivilege will call 'SQLGetDbmsSecurityB1Info()'
	// to check the B1-Security level:
	int hdl, nRes = RES_ERR ;
	TCHAR buf [MAXOBJECTNAME];
	if (GetDBNameFromObjType (OT_PROFILE, buf, NULL))
	{
		CString strSess;
		m_strVNodeName = (LPTSTR)GetVirtNodeName(m_nNodeHandle);
		strSess.Format (_T("%s::%s"), (LPCTSTR)m_strVNodeName, (LPTSTR)buf);
		nRes = Getsession ((LPUCHAR)(LPCTSTR)strSess, SESSION_TYPE_CACHEREADLOCK, &hdl);
		if (nRes != RES_SUCCESS)
			TRACE0 (_T("CaProfile::CaProfile(): Fail to Get the session\n"));
	}
	InitPrivilege();
	if (nRes != RES_ERR)
		ReleaseSession (hdl, RELEASE_COMMIT);

	if (!m_strNoGroup.LoadString (IDS_I_NOGROUP))
		m_strNoGroup = _T("(no group)");
	if (!m_strNoProfile.LoadString (IDS_I_NOPROFILE))
		m_strNoProfile = _T("(no profile)");
	if (m_pParam)
	{
		PROFILEPARAMS* pProfile = (PROFILEPARAMS*)m_pParam;
		Copy2ProfileObject (pProfile, this);
	}
}



CaProfile::~CaProfile()
{
	while (!m_listPrivileges.IsEmpty())
		delete m_listPrivileges.RemoveHead();

	if (m_Privilege)
		delete [] m_Privilege;
	if (m_bDefaultPrivilege)
		delete [] m_bDefaultPrivilege;
	if (m_bSecAudit)
		delete [] m_bSecAudit;
}

int  CaProfile::GetMaxPrivilege() {return USERPRIVILEGES;}
BOOL CaProfile::GetAllEvent() {return m_bSecAudit[USERALLEVENT];}
BOOL CaProfile::GetQueryText() { return m_bSecAudit[USERQUERYTEXTEVENT];}
void CaProfile::SetAllEvent(BOOL bFlag)  {m_bSecAudit[USERALLEVENT] = bFlag;}
void CaProfile::SetQueryText(BOOL bFlag) { m_bSecAudit[USERQUERYTEXTEVENT] = bFlag;}


void CaProfile::PushHelpID()
{
	PushHelpId (m_bCreate? IDD_PROFILE: IDD_ALTERPROFILE);
}

void CaProfile::PopHelpID()
{
	PopHelpId ();
}


CString CaProfile::GetTitle()
{
	CString strTitle;
	CString strTitleFmt;
	if (m_bCreate)
		strTitleFmt.LoadString  (IDS_T_CREATE_PROFILE);
	else
		strTitleFmt.LoadString  (IDS_T_ALTER_PROFILE);
	ASSERT (m_nNodeHandle != -1);
	m_strVNodeName = (m_nNodeHandle == -1)? _T("Error"): (LPCTSTR)GetVirtNodeName(m_nNodeHandle);
	strTitle.Format (strTitleFmt, (LPCTSTR)m_strVNodeName);
	return strTitle;
}

void CaProfile::GetGroups (CStringList& listGroup)
{
#if defined (ACCESS_LOWLEVELCALL)
	int   hdl, ires;
	BOOL  bwsystem;
	TCHAR buf [MAXOBJECTNAME];
	TCHAR buffilter [MAXOBJECTNAME];

	memset (buf, 0, sizeof(buf));
	memset (buffilter, 0, sizeof(buf));

	hdl = m_nNodeHandle;
	bwsystem = GetSystemFlag ();
	ires = DOMGetFirstObject (hdl, OT_GROUP, 0, NULL, bwsystem, NULL, (LPUCHAR)buf, NULL, NULL);
	while (ires == RES_SUCCESS)
	{
		listGroup.AddTail ((LPTSTR)buf);
		ires = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, NULL);
	}
#else
	listGroup.AddTail ("Gr1");
	listGroup.AddTail ("Gr2");
	listGroup.AddTail ("Gr3");
	listGroup.AddTail ("Gr4");
#endif
}


static BOOL Copy2ProfileParam (CaProfile* pProfileObject, PROFILEPARAMS* pProfileParam)
{
	BOOL bOk = TRUE;
#if defined (ACCESS_LOWLEVELCALL)
	int i;

	pProfileParam->bDefProfile = pProfileObject->m_bDefaultProfile;

	if (pProfileObject->m_strDefaultGroup.CompareNoCase(pProfileObject->m_strNoGroup) == 0)
		pProfileObject->m_strDefaultGroup = _T("");
	//
	// Security audit:
	for (i=0; i<USERSECAUDITS; i++)
		pProfileParam->bSecAudit [i] = pProfileObject->m_bSecAudit[i];
	//
	// Privilege and Default privileges:
	for (i=0; i<USERPRIVILEGES; i++)
	{
		pProfileParam->Privilege [i] = pProfileObject->m_Privilege[i];
		pProfileParam->bDefaultPrivilege [i] = pProfileObject->m_bDefaultPrivilege[i];
	}
	//
	// Name, Group,
	lstrcpy ((LPTSTR)pProfileParam->ObjectName,    pProfileObject->m_strName);
	lstrcpy ((LPTSTR)pProfileParam->DefaultGroup,  pProfileObject->m_strDefaultGroup);
	//
	// Limit Security Label:
	CString m_strExpireDate;
	
	if (pProfileParam->lpLimitSecLabels)
	{
		ESL_FreeMem (pProfileParam->lpLimitSecLabels);
		pProfileParam->lpLimitSecLabels  = NULL;
	}

	if (!pProfileObject->m_strLimitSecLabels.IsEmpty())
	{
		pProfileParam->lpLimitSecLabels = (LPUCHAR)ESL_AllocMem (pProfileObject->m_strLimitSecLabels.GetLength() +1);
		if (pProfileParam->lpLimitSecLabels)
			lstrcpy ((LPTSTR)pProfileParam->lpLimitSecLabels, pProfileObject->m_strLimitSecLabels);
		else
			pProfileParam->lpLimitSecLabels = NULL;
	}
	//
	// Expire Date:
	lstrcpy ((LPTSTR)pProfileParam->ExpireDate, pProfileObject->m_strExpireDate);
#endif
	return bOk;
}

static BOOL Copy2ProfileObject (PROFILEPARAMS* pProfileParam, CaProfile* pProfileObject)
{
	BOOL bOk = TRUE;
#if defined (ACCESS_LOWLEVELCALL)
	int i;
	pProfileObject->m_bDefaultProfile = pProfileParam->bDefProfile;
	//
	// Security audit:
	for (i=0; i<USERSECAUDITS; i++)
		pProfileObject->m_bSecAudit[i] = pProfileParam->bSecAudit [i];
	//
	// Privilege and Default privileges:
	for (i=0; i<USERPRIVILEGES; i++)
	{
		pProfileObject->m_Privilege[i] = pProfileParam->Privilege [i];
		pProfileObject->m_bDefaultPrivilege[i] = pProfileParam->bDefaultPrivilege [i];
	}
	//
	// Name, Group,
	pProfileObject->m_strName = (LPTSTR)pProfileParam->ObjectName;
	pProfileObject->m_strDefaultGroup = (LPTSTR)pProfileParam->DefaultGroup;
	//
	// Limit Security Label:
	pProfileObject->m_strLimitSecLabels = (pProfileParam->lpLimitSecLabels)? (LPTSTR)pProfileParam->lpLimitSecLabels: _T("");
	//
	// Expire Date:
	pProfileObject->m_strExpireDate = (LPTSTR)pProfileParam->ExpireDate;

	if (pProfileObject->m_strDefaultGroup.CompareNoCase(pProfileObject->m_strNoGroup) == 0)
		pProfileObject->m_strDefaultGroup = _T("");
#endif
	return bOk;
}

BOOL CaProfile::CreateObject (CWnd* pWnd)
{
	BOOL bRet = TRUE;
#if defined (ACCESS_LOWLEVELCALL)
	int ires = RES_ERR;
	//
	// Construct the user param:
	PROFILEPARAMS profile;
	memset (&profile, 0, sizeof(profile));
	if (m_pParam == NULL)
		m_pParam = (void*)&profile;
	if (Copy2ProfileParam (this, (PROFILEPARAMS*)m_pParam))
		ires = DBAAddObject ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_PROFILE, m_pParam);

	if (ires != RES_SUCCESS)
	{
		ErrorMessage ((UINT) IDS_E_CREATE_PROFILE_FAILED, ires);
		bRet = FALSE;
	}
#endif
	return bRet;
}

BOOL CaProfile::AlterObject  (CWnd* pWnd)
{
	BOOL Success = TRUE, bOverwrite = FALSE;
#if defined (ACCESS_LOWLEVELCALL)
	PROFILEPARAMS profile2;
	PROFILEPARAMS profile3;
	int  ires;
	int  hdl = m_nNodeHandle;
	PROFILEPARAMS* lpprofile1= (PROFILEPARAMS*)m_pParam;
	BOOL bChangedbyOtherMessage = FALSE;
	int irestmp;
	memset (&profile2, 0, sizeof(profile2));
	memset (&profile3, 0, sizeof(profile3));

	lstrcpy ((LPTSTR)profile2.ObjectName, (LPCTSTR)lpprofile1->ObjectName);
	lstrcpy ((LPTSTR)profile3.ObjectName, (LPCTSTR)lpprofile1->ObjectName);
	Success = TRUE;
	ires    = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_PROFILE, (void*)&profile2, TRUE, &hdl);
	if (ires != RES_SUCCESS)
	{
		ReleaseSession(hdl, RELEASE_ROLLBACK);
		switch (ires) 
		{
		case RES_ENDOFDATA:
			ires = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_CREATE);
			if (ires == IDYES) 
			{
				//
				// Fill structure:
				Success = Copy2ProfileParam (this, &profile3);
				if (Success) 
				{
					ires = DBAAddObject ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_PROFILE, (void *)&profile3);
					if (ires != RES_SUCCESS) 
					{
						ErrorMessage ((UINT) IDS_E_CREATE_PROFILE_FAILED, ires);
						Success = FALSE;
					}
				}
			}
			break;
		default:
			Success=FALSE;
			ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			break;
		}
		FreeAttachedPointers ((void *)&profile2, OT_PROFILE);
		FreeAttachedPointers ((void *)&profile3, OT_PROFILE);
		return Success;
	}

	if (!AreObjectsIdentical ((void *)lpprofile1, (void *)&profile2, OT_PROFILE))
	{
		ReleaseSession(hdl, RELEASE_ROLLBACK);
		ires = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
		bChangedbyOtherMessage = TRUE;
		irestmp = ires;
		if (ires == IDYES) 
		{
			ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_PROFILE, (void *)lpprofile1, FALSE, &hdl);
			if (ires != RES_SUCCESS) 
			{
				Success =FALSE;
				ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			}
			else 
			{
				//
				// Fill control:
				Copy2ProfileObject (lpprofile1, this);
				((CxDlgProfile*)pWnd)->InitClassMembers();
				((CxDlgProfile*)pWnd)->UpdateData (FALSE);
				FreeAttachedPointers ((void *)&profile2, OT_PROFILE);
				return FALSE;
			}
		}
		else
		{
			// Double Confirmation ?
			ires = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_OVERWRITE);
			if (ires != IDYES)
				Success=FALSE;
			else
			{
				TCHAR buf [MAXOBJECTNAME];
				if (GetDBNameFromObjType (OT_PROFILE, buf, NULL))
				{
					CString strSess;
					strSess.Format (_T("%s::%s"), (LPCTSTR)m_strVNodeName, (LPTSTR)buf);
					ires = Getsession ((LPUCHAR)(LPCTSTR)strSess, SESSION_TYPE_CACHEREADLOCK, &hdl);
					if (ires != RES_SUCCESS)
						Success = FALSE;
				}
				else
					Success = FALSE;
			}
		}
		if (!Success)
		{
			FreeAttachedPointers ((void*)&profile2, OT_PROFILE);
			return Success;
		}
	}

	//
	// Fill structure:
	Success = Copy2ProfileParam (this, &profile3);
	if (Success) 
	{
		ires = DBAAlterObject ((void*)lpprofile1, (void*)&profile3, OT_PROFILE, hdl);
		if (ires != RES_SUCCESS)  
		{
			Success=FALSE;
			ErrorMessage ((UINT) IDS_E_ALTER_PROFILE_FAILED, ires);
			ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_PROFILE, (void*)&profile2, FALSE, &hdl);
			if (ires != RES_SUCCESS)
				ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			else 
			{
				if (!AreObjectsIdentical ((void*)lpprofile1, (void*)&profile2, OT_PROFILE)) 
				{
					if (!bChangedbyOtherMessage) 
					{
						ires  = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
					}
					else
						ires=irestmp;

					if (ires == IDYES) 
					{
						ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_PROFILE, (void*)lpprofile1, FALSE, &hdl);
						if (ires == RES_SUCCESS)
						{
							//
							// Fill controls:
							Copy2ProfileObject (lpprofile1, this);
							((CxDlgProfile*)pWnd)->InitClassMembers();
							((CxDlgProfile*)pWnd)->UpdateData (FALSE);
						}
					}
					else
						bOverwrite = TRUE;
				}
			}
		}
	}

	FreeAttachedPointers ((void*)&profile2, OT_PROFILE);
	FreeAttachedPointers ((void*)&profile3, OT_PROFILE);
#endif
	return Success;
}


extern "C" BOOL WINAPI __export DlgCreateProfile (HWND hwndOwner, LPPROFILEPARAMS lpprofile)
{
	int nNodeHandle = GetCurMdiNodeHandle();
	if (nNodeHandle == -1)
		return 0;

	CaProfile profile (nNodeHandle, lpprofile->bCreate, (void*)lpprofile);
	if (!lpprofile->bCreate)
		Copy2ProfileObject (lpprofile, &profile);
	profile.m_strVNodeName = (LPTSTR)GetVirtNodeName(nNodeHandle);

	CxDlgProfile dlg;
	dlg.SetParam (&profile);

	return dlg.DoModal();
}


//
// ******************************************** END PROFILE ********************************






// ----------------------------------------------------------
// CREATE / ALTER ROLE
// ----------------------------------------------------------

CaRole::CaRole(int nNodeHandle, BOOL bCreate, void* lpRoleParam)
	: CaProfile(nNodeHandle, bCreate, NULL)
{
	m_pParam = lpRoleParam;
	if (m_pParam)
	{
		ROLEPARAMS* pRole = (ROLEPARAMS*)m_pParam;
		Copy2RoleObject (pRole, this);
	}
}


CaRole::~CaRole()
{
}

void CaRole::PushHelpID()
{
	PushHelpId (m_bCreate? IDD_ROLE: IDD_ALTERROLE);
}


CString CaRole::GetTitle()
{
	CString strTitle;
	CString strTitleFmt;
	if (m_bCreate)
		strTitleFmt.LoadString  (IDS_T_CREATE_ROLE);
	else
		strTitleFmt.LoadString  (IDS_T_ALTER_ROLE);
	ASSERT (m_nNodeHandle != -1);
	m_strVNodeName = (m_nNodeHandle == -1)? _T("Error"): (LPCTSTR)GetVirtNodeName(m_nNodeHandle);
	strTitle.Format (strTitleFmt, (LPCTSTR)m_strVNodeName);
	return strTitle;
}

void CaRole::GetDatabases (CStringList& listDatabase)
{
#if defined (ACCESS_LOWLEVELCALL)
	int   hdl, ires;
	BOOL  bwsystem;
	TCHAR buf [MAXOBJECTNAME];
	TCHAR buffilter [MAXOBJECTNAME];

	memset (buf, 0, sizeof(buf));
	memset (buffilter, 0, sizeof(buf));

	hdl = m_nNodeHandle;
	bwsystem = GetSystemFlag ();
	ires = DOMGetFirstObject (hdl, OT_DATABASE, 0, NULL, bwsystem, NULL, (LPUCHAR)buf, NULL, NULL);
	while (ires == RES_SUCCESS)
	{
		listDatabase.AddTail ((LPTSTR)buf);
		ires  = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, NULL);
	}
#else
	listDatabase.AddTail ("Db1");
	listDatabase.AddTail ("Db2");
	listDatabase.AddTail ("Db3");
	listDatabase.AddTail ("Db4");
#endif
}

static BOOL Copy2RoleParam (CaRole* pRoleObject, ROLEPARAMS* pRoleParam)
{
	BOOL bOk = TRUE;
#if defined (ACCESS_LOWLEVELCALL)
	int i;
	//
	// Security audit:
	for (i=0; i<USERSECAUDITS; i++)
		pRoleParam->bSecAudit [i] = pRoleObject->m_bSecAudit[i];
	//
	// External Password:
	pRoleParam->bExtrnlPassword = pRoleObject->m_bExternalPassword;
	//
	// Privilege and Default privileges:
	for (i=0; i<USERPRIVILEGES; i++)
	{
		pRoleParam->Privilege [i] = pRoleObject->m_Privilege[i];
		pRoleParam->bDefaultPrivilege [i] = pRoleObject->m_bDefaultPrivilege[i];
	}
	//
	// Name:
	lstrcpy ((LPTSTR)pRoleParam->ObjectName,    pRoleObject->m_strName);
	//
	// Limit Security Label:
	CString m_strExpireDate;
	
	if (pRoleParam->lpLimitSecLabels)
	{
		ESL_FreeMem (pRoleParam->lpLimitSecLabels);
		pRoleParam->lpLimitSecLabels  = NULL;
	}

	if (!pRoleObject->m_strLimitSecLabels.IsEmpty())
	{
		pRoleParam->lpLimitSecLabels = (LPUCHAR)ESL_AllocMem (pRoleObject->m_strLimitSecLabels.GetLength() +1);
		if (pRoleParam->lpLimitSecLabels)
			lstrcpy ((LPTSTR)pRoleParam->lpLimitSecLabels, pRoleObject->m_strLimitSecLabels);
		else
			pRoleParam->lpLimitSecLabels = NULL;
	}
	//
	// Password
	if (!pRoleParam->bExtrnlPassword)
		lstrcpy ((LPTSTR)pRoleParam->szPassword, pRoleObject->m_strPassword);
	//
	// List of the Databases:
	LPCHECKEDOBJECTS obj = NULL, list = NULL;

	POSITION pos = pRoleObject->m_strListDatabase.GetHeadPosition();
	while (pos != NULL)
	{
		CString strObj = pRoleObject->m_strListDatabase.GetNext(pos);
		obj = (CHECKEDOBJECTS*)ESL_AllocMem (sizeof (CHECKEDOBJECTS));
		if (!obj)
		{
			list = FreeCheckedObjects (list);
			ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
			return FALSE;
		}
		else
		{
			lstrcpy ((LPTSTR)obj->dbname, strObj);
			obj->bchecked = TRUE;
			list = AddCheckedObject (list, obj);
		}
	}
	pRoleParam->lpfirstdb = list;
#endif
	return bOk;
}

static BOOL Copy2RoleObject (ROLEPARAMS* pRoleParam, CaRole* pRoleObject)
{
	BOOL bOk = TRUE;
#if defined (ACCESS_LOWLEVELCALL)
	int i;
	//
	// Security audit:
	for (i=0; i<USERSECAUDITS; i++)
		pRoleObject->m_bSecAudit[i] = pRoleParam->bSecAudit [i];
	//
	// External Password:
	pRoleObject->m_bExternalPassword = pRoleParam->bExtrnlPassword;
	//
	// Privilege and Default privileges:
	for (i=0; i<USERPRIVILEGES; i++)
	{
		pRoleObject->m_Privilege[i] = pRoleParam->Privilege [i];
		pRoleObject->m_bDefaultPrivilege[i] = pRoleParam->bDefaultPrivilege [i];
	}
	//
	// Name:
	pRoleObject->m_strName = (LPTSTR)pRoleParam->ObjectName;
	//
	// Limit Security Label:
	pRoleObject->m_strLimitSecLabels = (pRoleParam->lpLimitSecLabels)? (LPTSTR)pRoleParam->lpLimitSecLabels: _T("");
	//
	// Password
	if (!pRoleParam->bExtrnlPassword)
		pRoleObject->m_strPassword = (LPTSTR)pRoleParam->szPassword;
	else
		pRoleObject->m_strPassword = _T("");
	//
	// List of the Databases:
	pRoleObject->m_strListDatabase.RemoveAll();
	LPCHECKEDOBJECTS obj = NULL, list = NULL;
	list = pRoleParam->lpfirstdb;
	while (list)
	{
		obj = list;
		list = list->pnext;
		pRoleObject->m_strListDatabase.AddTail((LPTSTR)obj->dbname);
	}
#endif
	return bOk;
}


BOOL CaRole::CreateObject (CWnd* pWnd)
{
	BOOL bRet = TRUE;
#if defined (ACCESS_LOWLEVELCALL)
	int ires = RES_ERR;
	//
	// Construct the user param:
	ROLEPARAMS role;
	memset (&role, 0, sizeof(role));
	if (m_pParam == NULL)
		m_pParam = &role;
	if (Copy2RoleParam (this, (ROLEPARAMS*)m_pParam))
		ires = DBAAddObject ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_ROLE, m_pParam);

	if (ires != RES_SUCCESS)
	{
		ErrorMessage ((UINT) IDS_E_CREATE_ROLE_FAILED, ires);
		bRet = FALSE;
	}
	FreeAttachedPointers (m_pParam, OT_ROLE);
#endif
	return bRet;
}

BOOL CaRole::AlterObject (CWnd* pWnd)
{
	BOOL Success = TRUE, bOverwrite = FALSE;
#if defined (ACCESS_LOWLEVELCALL)
	ROLEPARAMS role2;
	ROLEPARAMS role3;
	int  ires;
	int  hdl = m_nNodeHandle;
	ASSERT (m_pParam);
	if (!m_pParam)
		return FALSE;

	ROLEPARAMS* lprole1 = (ROLEPARAMS*)m_pParam;
	BOOL bChangedbyOtherMessage = FALSE;
	int irestmp;
	memset (&role2, 0, sizeof(role2));
	memset (&role3, 0, sizeof(role3));

	lstrcpy ((LPTSTR)role2.ObjectName, (LPTSTR)lprole1->ObjectName);
	lstrcpy ((LPTSTR)role3.ObjectName, (LPTSTR)lprole1->ObjectName);
	Success = TRUE;
	
	ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_ROLE, &role2, TRUE, &hdl);
	if (ires != RES_SUCCESS)
	{
		ReleaseSession(hdl, RELEASE_ROLLBACK);
		switch (ires) 
		{
		case RES_ENDOFDATA:
			ires = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_CREATE);
			if (ires == IDYES) 
			{
				//
				// Fill structure:
				Success = Copy2RoleParam (this, &role3);
				if (Success) 
				{
					ires = DBAAddObject ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_ROLE, (void *)&role3);
					if (ires != RES_SUCCESS) 
					{
						ErrorMessage ((UINT) IDS_E_CREATE_ROLE_FAILED, ires);
						Success=FALSE;
					}
				}
			}
			break;
			 
		default:
			Success = FALSE;
			ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			break;
		}
		FreeAttachedPointers ((void*)&role2, OT_ROLE);
		FreeAttachedPointers ((void*)&role3, OT_ROLE);
		return Success;
	}

	if (!AreObjectsIdentical ((void*)lprole1, (void*)&role2, OT_ROLE))
	{
		ReleaseSession(hdl, RELEASE_ROLLBACK);
		ires = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
		bChangedbyOtherMessage = TRUE;
		irestmp=ires;
		if (ires == IDYES) 
		{
			FreeAttachedPointers ((void*)lprole1, OT_ROLE);
			ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_ROLE, lprole1, FALSE, &hdl);
			if (ires != RES_SUCCESS) 
			{
				Success =FALSE;
				ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			}
			else 
			{
				//
				// Fill controls:
				Copy2RoleObject (lprole1, this);
				((CxDlgRole*)pWnd)->InitClassMembers();
				((CxDlgRole*)pWnd)->UpdateData (FALSE);
				FreeAttachedPointers (&role2, OT_ROLE);
				return FALSE;
			}
		}
		else
		{
			// Double Confirmation ?
			ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_OVERWRITE);
			if (ires != IDYES)
				Success=FALSE;
			else
			{
				TCHAR buf [MAXOBJECTNAME];
				if (GetDBNameFromObjType (OT_ROLE, buf, NULL))
				{
					CString strSess;
					strSess.Format (_T("%s::%s"), (LPCTSTR)m_strVNodeName, (LPTSTR)buf);
					ires = Getsession ((LPUCHAR)(LPCTSTR)strSess, SESSION_TYPE_CACHEREADLOCK, &hdl);
					if (ires != RES_SUCCESS)
						Success = FALSE;
				}
				else
					Success = FALSE;
			}
		}

		if (!Success)
		{
			FreeAttachedPointers ((void*)&role2, OT_ROLE);
			return Success;
		}
	}

	//
	// Fill structure:
	Success = Copy2RoleParam (this, &role3);
	if (Success) 
	{
		ires = DBAAlterObject ((void*)lprole1, (void*)&role3, OT_ROLE, hdl);
		if (ires != RES_SUCCESS)  
		{
			Success=FALSE;
			ErrorMessage ((UINT) IDS_E_ALTER_ROLE_FAILED, ires);
			ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_ROLE, (void*)&role2, FALSE, &hdl);
			if (ires != RES_SUCCESS)
				ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			else 
			{
				if (!AreObjectsIdentical ((void*)lprole1, (void*)&role2, OT_ROLE)) 
				{
					if (!bChangedbyOtherMessage) 
					{
						ires = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
					}
					else
						ires = irestmp;

					if (ires == IDYES) 
					{
						ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_ROLE, (void*)lprole1, FALSE, &hdl);
						if (ires == RES_SUCCESS)
						{
							//
							// Fill controls:
							Copy2RoleObject (lprole1, this);
							((CxDlgRole*)pWnd)->InitClassMembers();
							((CxDlgRole*)pWnd)->UpdateData (FALSE);
						}
					}
					else
						bOverwrite = TRUE;
				}
			}
		}
	}

	FreeAttachedPointers ((void*)&role2, OT_ROLE);
	FreeAttachedPointers ((void*)&role3, OT_ROLE);
#endif
	return Success;
}



extern "C" int WINAPI __export DlgCreateRole (HWND hwndOwner, LPROLEPARAMS lprole)
{
	int nNodeHandle = GetCurMdiNodeHandle();
	if (nNodeHandle == -1)
		return 0;

	CaRole role (nNodeHandle, TRUE, (void*)lprole);
	role.m_strVNodeName = (LPTSTR)GetVirtNodeName(nNodeHandle);

	CxDlgRole dlg;
	dlg.SetParam (&role);

	return dlg.DoModal();
}

extern "C" int WINAPI __export DlgAlterRole (HWND hwndOwner, LPROLEPARAMS lprole)
{
	int nNodeHandle = GetCurMdiNodeHandle();
	if (nNodeHandle == -1)
		return 0;

	CaRole role (nNodeHandle, FALSE, (void*)lprole);
	role.m_strVNodeName = (LPTSTR)GetVirtNodeName(nNodeHandle);
	Copy2RoleObject (lprole, &role);
	
	CxDlgRole dlg (NULL, TRUE);
	dlg.SetParam (&role);

	return dlg.DoModal();
}



//
// ******************************************** END ROLE ********************************







// ----------------------------------------------------------
// CREATE / ALTER USER
// ----------------------------------------------------------

CaUser::CaUser(int nNodeHandle, BOOL bCreate, void* lpUserParam)
	:CaRole(nNodeHandle, bCreate, NULL)
{
	m_pParam = lpUserParam;
	if (m_pParam)
	{
		USERPARAMS* pUser = (USERPARAMS*)m_pParam;
		Copy2UserObject (pUser, this);
	}
}

CaUser::~CaUser()
{
}

void CaUser::PushHelpID()
{
	PushHelpId (m_bCreate? IDD_USER: IDD_ALTER_USER);
}


CString CaUser::GetTitle()
{
	CString strTitle;
	CString strTitleFmt;
	if (m_bCreate)
		strTitleFmt.LoadString  (IDS_T_CREATE_USER);
	else
		strTitleFmt.LoadString  (IDS_T_ALTER_USER);
	ASSERT (m_nNodeHandle != -1);
	m_strVNodeName = (m_nNodeHandle == -1)? _T("Error"): (LPCTSTR)GetVirtNodeName(m_nNodeHandle);
	strTitle.Format (strTitleFmt, (LPCTSTR)m_strVNodeName);
	return strTitle;
}


void CaUser::GetProfiles(CStringList& listProfile)
{
#if defined (ACCESS_LOWLEVELCALL)
	int   hdl, ires;
	BOOL  bwsystem;
	TCHAR buf [MAXOBJECTNAME];
	TCHAR buffilter [MAXOBJECTNAME];

	memset (buf, 0, sizeof(buf));
	memset (buffilter, 0, sizeof(buf));

	hdl = m_nNodeHandle;
	bwsystem = GetSystemFlag ();
	ires = DOMGetFirstObject (hdl, OT_PROFILE, 0, NULL, bwsystem, NULL, (LPUCHAR)buf, NULL, NULL);
	while (ires == RES_SUCCESS)
	{
		listProfile.AddTail ((LPTSTR)buf);
		ires = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, NULL);
	}
#else
	listProfile.AddTail ("Pro1");
	listProfile.AddTail ("Pro2");
	listProfile.AddTail ("Pro3");
	listProfile.AddTail ("Pro4");
#endif
}


static BOOL Copy2UserParam (CaUser* pUserObject, USERPARAMS* pUserParam)
{
	BOOL bOk = TRUE;
#if defined (ACCESS_LOWLEVELCALL)
	int i;
	//
	// Security audit:
	for (i=0; i<USERSECAUDITS; i++)
		pUserParam->bSecAudit [i] = pUserObject->m_bSecAudit[i];
	//
	// External Password:
	pUserParam->bExtrnlPassword = pUserObject->m_bExternalPassword;
	//Delete password
	pUserParam->bRemovePwd = pUserObject->m_bRemovePwd;
	//
	// Privilege and Default privileges:
	for (i=0; i<USERPRIVILEGES; i++)
	{
		pUserParam->Privilege [i] = pUserObject->m_Privilege[i];
		pUserParam->bDefaultPrivilege [i] = pUserObject->m_bDefaultPrivilege[i];
	}
	//
	// User Name, Group, profile:
	lstrcpy ((LPTSTR)pUserParam->ObjectName,    pUserObject->m_strName);
	lstrcpy ((LPTSTR)pUserParam->DefaultGroup,  pUserObject->m_strDefaultGroup);
	lstrcpy ((LPTSTR)pUserParam->szProfileName, pUserObject->m_strProfileName);
	//
	// Limit Security Label:
	if (pUserParam->lpLimitSecLabels)
	{
		ESL_FreeMem (pUserParam->lpLimitSecLabels);
		pUserParam->lpLimitSecLabels  = NULL;
	}

	if (!pUserObject->m_strLimitSecLabels.IsEmpty())
	{
		pUserParam->lpLimitSecLabels = (LPUCHAR)ESL_AllocMem (pUserObject->m_strLimitSecLabels.GetLength() +1);
		if (pUserParam->lpLimitSecLabels)
			lstrcpy ((LPTSTR)pUserParam->lpLimitSecLabels, pUserObject->m_strLimitSecLabels);
		else
			pUserParam->lpLimitSecLabels = NULL;
	}
	//
	// Password
	if (!pUserParam->bExtrnlPassword)
		lstrcpy ((LPTSTR)pUserParam->szPassword, pUserObject->m_strPassword);
	//
	// Expire Date:
	lstrcpy ((LPTSTR)pUserParam->ExpireDate, pUserObject->m_strExpireDate);

	//
	// List of the Databases:
	pUserParam->lpfirstdb = NULL;
	if (pUserObject->m_bCreate)
	{
		LPCHECKEDOBJECTS obj = NULL, list = NULL;
		POSITION pos = pUserObject->m_strListDatabase.GetHeadPosition();
		while (pos != NULL)
		{
			CString strObj = pUserObject->m_strListDatabase.GetNext(pos);
			obj = (CHECKEDOBJECTS*)ESL_AllocMem (sizeof (CHECKEDOBJECTS));
			if (!obj)
			{
				list = FreeCheckedObjects (list);
				ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
				return FALSE;
			}
			else
			{
				lstrcpy ((LPTSTR)obj->dbname, strObj);
				obj->bchecked = TRUE;
				list = AddCheckedObject (list, obj);
			}
		}
		pUserParam->lpfirstdb = list;
	}
#endif
	return bOk;
}

static BOOL Copy2UserObject (USERPARAMS* pUserParam, CaUser* pUserObject)
{
	BOOL bOk = TRUE;
#if defined (ACCESS_LOWLEVELCALL)
	int i;
	//
	// Security audit:
	for (i=0; i<USERSECAUDITS; i++)
		pUserObject->m_bSecAudit[i] = pUserParam->bSecAudit [i];
	//
	// External Password:
	pUserObject->m_bExternalPassword = pUserParam->bExtrnlPassword;

	// Delete Password:
	pUserObject->m_bRemovePwd = pUserParam->bRemovePwd;
	//
	// Privilege and Default privileges:
	for (i=0; i<USERPRIVILEGES; i++)
	{
		pUserObject->m_Privilege[i] = pUserParam->Privilege [i];
		pUserObject->m_bDefaultPrivilege[i] = pUserParam->bDefaultPrivilege [i];
	}
	//
	// User Name, Group, profile:
	pUserObject->m_strName = (LPTSTR)pUserParam->ObjectName;
	pUserObject->m_strDefaultGroup = (LPTSTR)pUserParam->DefaultGroup;
	pUserObject->m_strProfileName = (LPTSTR)pUserParam->szProfileName;
	//
	// Limit Security Label:
	pUserObject->m_strLimitSecLabels = (pUserParam->lpLimitSecLabels)? (LPTSTR)pUserParam->lpLimitSecLabels: _T("");
	//
	// Password
	if (!pUserParam->bExtrnlPassword)
		pUserObject->m_strPassword = (LPTSTR)pUserParam->szPassword;
	//
	// Expire Date:
	pUserObject->m_strExpireDate = (LPTSTR)pUserParam->ExpireDate;
	//
	// Old Password:
	pUserObject->m_strOldPassword = (LPTSTR)pUserParam->szOldPassword;
	//
	// List of the Databases:
	if (pUserObject->m_bCreate)
	{
		pUserObject->m_strListDatabase.RemoveAll();
		LPCHECKEDOBJECTS obj = NULL, list = NULL;
		list = pUserParam->lpfirstdb;
		while (list)
		{
			obj = list;
			list = list->pnext;
			pUserObject->m_strListDatabase.AddTail((LPTSTR)obj->dbname);
		}
	}
	pUserObject->m_bGrantUser4Rmcmd = pUserParam->bGrantUser4Rmcmd;
	pUserObject->m_bPartialGrant    = pUserParam->bPartialGrant;
	if(pUserParam->iOwnerRmcmdObjects == RMCMDOBJETS_OWNER)
		pUserObject->m_bOwnerRmcmdObjects = TRUE;
	else
		pUserObject->m_bOwnerRmcmdObjects = FALSE;
#endif
	return bOk;
}

BOOL CaUser::CreateObject (CWnd* pWnd)
{
	BOOL bRet = TRUE;
#if defined (ACCESS_LOWLEVELCALL)
	int ires = RES_ERR;
	//
	// Construct the user param:
	USERPARAMS user;
	memset (&user, 0, sizeof(user));
	if (m_pParam == NULL)
		m_pParam = &user;
	if (Copy2UserParam (this, (USERPARAMS*)m_pParam))
		ires = DBAAddObject ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_USER, m_pParam);

	if (ires != RES_SUCCESS)
	{
		ErrorMessage ((UINT) IDS_E_CREATE_USER_FAILED, ires);
		bRet = FALSE;
	}
	FreeAttachedPointers (m_pParam, OT_USER);
#endif
	return bRet;
}

BOOL CaUser::AlterObject (CWnd* pWnd)
{
	BOOL Success = TRUE, bOverwrite = FALSE;
#if defined (ACCESS_LOWLEVELCALL)
	USERPARAMS user2;
	USERPARAMS user3;
	int  ires;
	int  hdl = m_nNodeHandle;
	USERPARAMS* lpuser1 = (USERPARAMS*)m_pParam;
	BOOL bChangedbyOtherMessage = FALSE;
	int irestmp;
	memset (&user2, 0, sizeof(user2));
	memset (&user3, 0, sizeof(user3));

	lstrcpy ((LPTSTR)user2.ObjectName, (LPCTSTR)lpuser1->ObjectName);
	lstrcpy ((LPTSTR)user3.ObjectName, (LPCTSTR)lpuser1->ObjectName);
	Success = TRUE;
	ires    = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_USER, (void*)&user2, TRUE, &hdl);
	if (ires != RES_SUCCESS)
	{
		ReleaseSession(hdl,RELEASE_ROLLBACK);
		switch (ires) 
		{
		case RES_ENDOFDATA:
			ires = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_CREATE);
			if (ires == IDYES) 
			{
				//
				// Fill structure:
				Success = Copy2UserParam (this, &user3);
				if (Success) 
				{
					ires = DBAAddObject ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_USER, (void *)&user3 );
					if (ires != RES_SUCCESS) 
					{
						ErrorMessage ((UINT) IDS_E_CREATE_USER_FAILED, ires);
						Success=FALSE;
					}
				}
			}
			break;
		default:
			Success=FALSE;
			ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			break;
		}
		FreeAttachedPointers ((void*)&user2, OT_USER);
		FreeAttachedPointers ((void*)&user3, OT_USER);
		return Success;
	}

	if (!AreObjectsIdentical ((void*)lpuser1, (void*)&user2, OT_USER))
	{
		ReleaseSession(hdl, RELEASE_ROLLBACK);
		ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
		bChangedbyOtherMessage = TRUE;
		irestmp=ires;
		if (ires == IDYES) 
		{
			ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_USER, (void*)lpuser1, FALSE, &hdl);
			if (ires != RES_SUCCESS) 
			{
				Success =FALSE;
				ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			}
			else 
			{
				//
				// Fill controls:
				Copy2UserObject (lpuser1, this);
				((CxDlgUser*)pWnd)->InitClassMember();
				((CxDlgUser*)pWnd)->UpdateData(FALSE);
				FreeAttachedPointers ((void*)&user2, OT_USER);
				return FALSE;  
			}
		}
		else
		{
			// Double Confirmation ?
			ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_OVERWRITE);
			if (ires != IDYES)
				Success=FALSE;
			else
			{
				TCHAR buf [MAXOBJECTNAME];
				if (GetDBNameFromObjType (OT_USER,  buf, NULL))
				{
					CString strSess;
					strSess.Format (_T("%s::%s"), (LPCTSTR)m_strVNodeName, (LPTSTR)buf);
					ires = Getsession ((LPUCHAR)(LPCTSTR)strSess, SESSION_TYPE_CACHEREADLOCK, &hdl);
					if (ires != RES_SUCCESS)
						Success = FALSE;
				}
				else
					Success = FALSE;
			}
		}
		if (!Success)
		{
			FreeAttachedPointers ((void*)&user2, OT_USER);
			return Success;
		}
	}

	//
	// Fill structure:
	Success = Copy2UserParam (this, &user3);
	if (Success) 
	{
		ires = DBAAlterObject ((void*)lpuser1, (void*)&user3, OT_USER, hdl);
		if (ires != RES_SUCCESS)
		{
			Success=FALSE;
			ErrorMessage ((UINT) IDS_E_ALTER_USER_FAILED, ires);
			ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_USER, (void*)&user2, FALSE, &hdl);
			if (ires != RES_SUCCESS)
				ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			else 
			{
				if (!AreObjectsIdentical ((void*)lpuser1, (void*)&user2, OT_USER))
				{
					if (!bChangedbyOtherMessage) 
					{
						ires  = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
					}
					else
						ires=irestmp;

					if (ires == IDYES) 
					{
						ires = GetDetailInfo ((LPUCHAR)(LPCTSTR)m_strVNodeName, OT_USER, (void*)lpuser1, FALSE, &hdl);
						if (ires == RES_SUCCESS)
						{
							//
							// Fill controls:
							Copy2UserObject (lpuser1, this);
							((CxDlgUser*)pWnd)->InitClassMember();
							((CxDlgUser*)pWnd)->UpdateData(FALSE);
						}
					}
					else
						bOverwrite = TRUE;
				}
			}
		}
	}

	FreeAttachedPointers ((void*)&user2, OT_USER);
	FreeAttachedPointers ((void*)&user3, OT_USER);
#endif
	return Success;
}

BOOL CaUser::GrantRevoke4Rmcmd(CWnd* pWnd)
{
	USERPARAMS* lpuser1 = (USERPARAMS*)m_pParam;
	TCHAR szConnectName[3*MAXOBJECTNAME];
	TCHAR *pUserName;
	LPTSTR *TabTemp = NULL;
	CString csStatement;
	int iret,iNum,iSession,iMAX ;
	BOOL bDisplayErrorMess;
	TCHAR * TabRevokeRmcmd[]={ "revoke select,insert,update,delete on $ingres.remotecmdinview from user %s restrict",
	                           "revoke select,insert,update,delete on $ingres.remotecmdoutview from user %s restrict",
	                           "revoke select,insert,update,delete on $ingres.remotecmdview from user %s restrict",
	                           "revoke execute on procedure $ingres.launchremotecmd from user %s restrict",
	                           "revoke execute on procedure $ingres.sendrmcmdinput from user %s restrict",
	                           "revoke register,raise on dbevent $ingres.rmcmdcmdend from user %s restrict",
	                           "revoke register,raise on dbevent $ingres.rmcmdnewcmd from user %s restrict",
	                           "revoke register,raise on dbevent $ingres.rmcmdnewinputline from user %s restrict",
	                           "revoke register,raise on dbevent $ingres.rmcmdnewoutputline from user %s restrict",
	                           "revoke register,raise on dbevent $ingres.rmcmdstp from user %s restrict"
	                         };

	TCHAR * TabGranteRmcmd[]={ "grant select,insert,update,delete on $ingres.remotecmdinview to %s",
	                           "grant select,insert,update,delete on $ingres.remotecmdoutview to %s",
	                           "grant select,insert,update,delete on $ingres.remotecmdview to %s",
	                           "grant execute on procedure $ingres.launchremotecmd to %s",
	                           "grant execute on procedure $ingres.sendrmcmdinput to %s",
	                           "grant register,raise on dbevent $ingres.rmcmdcmdend to %s",
	                           "grant register,raise on dbevent $ingres.rmcmdnewcmd to %s",
	                           "grant register,raise on dbevent $ingres.rmcmdnewinputline to %s",
	                           "grant register,raise on dbevent $ingres.rmcmdnewoutputline to %s",
	                           "grant register,raise on dbevent $ingres.rmcmdstp to %s"
	                         };

	if (lpuser1->bGrantUser4Rmcmd != m_bGrantUser4Rmcmd )
	{
		if ( m_bGrantUser4Rmcmd)
		{
			TabTemp = TabGranteRmcmd;
			iMAX = sizeof(TabGranteRmcmd)/sizeof(TabGranteRmcmd[0]);
		}
		else
		{
			TabTemp = TabRevokeRmcmd;
			iMAX = sizeof(TabRevokeRmcmd)/sizeof(TabRevokeRmcmd[0]);
		}
	}
	else
		return TRUE;

	pUserName = (LPTSTR)QuoteIfNeeded((LPCTSTR)lpuser1->ObjectName);

	if ( GetOIVers() >= OIVERS_26)
		wsprintf(szConnectName,"%s::imadb",(LPUCHAR)(LPCTSTR)m_strVNodeName);
	else
		wsprintf(szConnectName,"%s::iidbdb",(LPUCHAR)(LPCTSTR)m_strVNodeName);

	iret = Getsession((LPUCHAR)szConnectName,SESSION_TYPE_INDEPENDENT, &iSession);
	if (iret != RES_SUCCESS)
	{
		ErrorMessage((UINT) IDS_E_ERROR_RMCMD_GRANT, iret);
		return FALSE;
	}

	bDisplayErrorMess = FALSE;
	for (iNum = 0; iNum < iMAX; iNum++)
	{
		csStatement.Format(TabTemp[iNum],pUserName);
		iret=ExecSQLImm((LPUCHAR)(LPCTSTR)csStatement,FALSE, NULL, NULL, NULL);
		if (iret!=RES_SUCCESS || isLastErr50())
		{
			bDisplayErrorMess = TRUE;
			break;
		}
	}

	if (iret==RES_SUCCESS && !bDisplayErrorMess)
	{
		ReleaseSession(iSession, RELEASE_COMMIT);
		return TRUE;
	}
	else
	{
		ReleaseSession(iSession, RELEASE_ROLLBACK);
		ErrorMessage ((UINT) IDS_E_ERROR_RMCMD_GRANT, iret);
		return FALSE;
	}
}


extern "C" int WINAPI __export DlgCreateUser (HWND hwndOwner, LPUSERPARAMS lpuser)
{
	int nNodeHandle = GetCurMdiNodeHandle();
	if (nNodeHandle == -1)
		return 0;

	CaUser user (nNodeHandle, TRUE, (void*)lpuser);
	user.m_strVNodeName = (LPTSTR)GetVirtNodeName(nNodeHandle);

	CxDlgUser dlg;
	dlg.SetParam (&user);

	return dlg.DoModal();
}


extern "C" int WINAPI __export DlgAlterUser (HWND hwndOwner, LPUSERPARAMS lpuserparams)
{
	char szConnectName[MAXOBJECTNAME*3];
	int iret,iSession,nNodeHandle = GetCurMdiNodeHandle();
	if (nNodeHandle == -1)
		return 0;

	CaUser user (nNodeHandle, FALSE, (void*)lpuserparams);
	user.m_strVNodeName = (LPTSTR)GetVirtNodeName(nNodeHandle);

	if ( GetOIVers() >= OIVERS_26)
		wsprintf(szConnectName,"%s::imadb",(LPUCHAR)(LPCTSTR)user.m_strVNodeName);
	else
		wsprintf(szConnectName,"%s::iidbdb",(LPUCHAR)(LPCTSTR)user.m_strVNodeName);

	iret = Getsession((LPUCHAR)szConnectName,SESSION_TYPE_INDEPENDENT, &iSession);
	if (iret != RES_SUCCESS)
	{
		ErrorMessage((UINT) IDS_E_GET_SESSION, iret);
		return FALSE;
	}
	iret = GetInfGrantUser4Rmcmd(lpuserparams);
	if (iret==RES_SUCCESS)
		ReleaseSession(iSession, RELEASE_COMMIT);
	else
	{
		ReleaseSession(iSession, RELEASE_ROLLBACK);
		ErrorMessage ((UINT) IDS_E_RETRIEVE_RMCMD_GRANT_FAILED, iret);
	}

	Copy2UserObject (lpuserparams, &user);

	CxDlgUser dlg (NULL, TRUE);
	dlg.SetParam (&user);

	return dlg.DoModal();
}
