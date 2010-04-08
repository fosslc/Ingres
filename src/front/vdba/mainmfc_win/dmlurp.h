/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dmlurp.h, Header File 
**    Project  : Ingres II / VDBA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data manipulation (User, Role, Profile)
**
** History:
**
** 26-Oct-1999 (uk$so01)
**    Created, Rewrite the dialog code, in C++/MFC.
** 24-Feb-2000 (uk$so01)
**    Bug #100557: Remove the (default profile) from the create user dialog box
** 14-Sep-2000 (uk$so01)
**    Bug #102609: Pop help ID when destroying the dialog
** 15-Jun-2001(schph01)
**    SIR 104991 add definition for method GrantRevoke4Rmcmd()
** 07-Oct-2002(schph01)
**    SIR 108883 add variable member m_bOwnerRmcmdObjects in CaUser class
** 07-Apr-2005 (srisu02)
**   Add variable member m_bRemovePwd in CaUser class
**/


#if !defined (DMLURP_HEADER)
#define DMLURP_HEADER

BOOL INGRESII_IsDefaultProfile(LPCTSTR lpszTring);

// 
// PROFILE OBJECT
// ----------------------------------------------------------
class CaURPPrivilegesItemData;
class CaProfile: public CObject
{
public:
	CaProfile(int nNodeHandle, BOOL bCreate, void* lpProfileParam);
	~CaProfile();

	void PushHelpID();
	void PopHelpID();
	CString GetTitle();
	void GetGroups  (CStringList& listGroup);
	BOOL CreateObject (CWnd* pWnd);
	BOOL AlterObject  (CWnd* pWnd);

	int  GetMaxPrivilege();
	BOOL GetAllEvent();
	BOOL GetQueryText();
	void SetAllEvent(BOOL bFlag);
	void SetQueryText(BOOL bFlag);

	//
	// User Param:
	CString m_strName;
	CString m_strDefaultGroup; // Can be empty

	CString m_strExpireDate;
	CString m_strLimitSecLabels;

	BOOL*   m_Privilege;
	BOOL*   m_bDefaultPrivilege;
	BOOL*   m_bSecAudit;

	BOOL    m_bDefaultProfile;

	//
	// Internal use:
	int  m_nNodeHandle;
	BOOL m_bCreate;

	CString m_strVNodeName;
	CString m_strNoGroup;
	CString m_strNoProfile;
	CTypedPtrList< CObList, CaURPPrivilegesItemData* > m_listPrivileges;
protected:
	void* m_pParam;

	void InitPrivilege();
};




// 
// ROLE OBJECT
// ----------------------------------------------------------
class CaRole: public CaProfile
{
public:
	CaRole(int nNodeHandle, BOOL bCreate, void* lpUserParam);
	~CaRole();

	void PushHelpID();
	CString GetTitle();
	void GetDatabases (CStringList& listDatabase);
	BOOL CreateObject (CWnd* pWnd);
	BOOL AlterObject  (CWnd* pWnd);

	CString m_strPassword;
	CString m_strOldPassword;
	BOOL    m_bExternalPassword;
	CStringList m_strListDatabase;
};



// 
// USER OBJECT
// ----------------------------------------------------------
class CaUser: public CaRole
{
public:
	BOOL GrantRevoke4Rmcmd(CWnd* pWnd);
	CaUser(int nNodeHandle, BOOL bCreate, void* lpUserParam);
	~CaUser();

	void PushHelpID();
	CString GetTitle();
	void GetProfiles(CStringList& listProfile);
	BOOL CreateObject (CWnd* pWnd);
	BOOL AlterObject  (CWnd* pWnd);

	//
	// User Param:
	CString m_strProfileName;  // Can be empty
	CString m_strOldPassword;

	BOOL m_bGrantUser4Rmcmd;   // TRUE defined grants for Rmcmd command,FALSE REVOKE
	BOOL m_bPartialGrant;      // TRUE only partial grants are defined for this user
	BOOL m_bOwnerRmcmdObjects; // TRUE owner of all rmcmd objects ( tables, views, procedures )
	BOOL m_bRemovePwd;        // TRUE Flag to determine if users password needs to be deleted
};



#endif


